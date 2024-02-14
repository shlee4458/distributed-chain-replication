#include <iostream>
#include <memory>
#include <map>

#include "ServerThread.h"
#include "ServerStub.h"
#include "Messages.h"

#define PFA_IDENTIFIER 1
#define CUSTOMER_IDENTIFIER 2
#define UPDATE_REQUEST 1
#define READ_REQUEST 2

LaptopInfo LaptopFactory::
GetLaptopInfo(CustomerRequest request, int engineer_id) {
	LaptopInfo laptop;
	laptop.CopyRequest(request);
	laptop.SetEngineerId(engineer_id);
	laptop.SetAdminId(-1);
	return laptop;
}

LaptopInfo LaptopFactory::
CreateLaptop(CustomerRequest request, int engineer_id) {
	LaptopInfo laptop;
	laptop.CopyRequest(request);
	laptop.SetEngineerId(engineer_id);

	std::promise<LaptopInfo> prom;
	std::future<LaptopInfo> fut = prom.get_future();

	std::unique_ptr<PrimaryAdminRequest> req = 
		std::unique_ptr<PrimaryAdminRequest>(new PrimaryAdminRequest);
	req->laptop = laptop;
	req->prom = std::move(prom);

	erq_lock.lock();
	erq.push(std::move(req));
	erq_cv.notify_one();
	erq_lock.unlock();

	laptop = fut.get();
	return laptop;
}

void LaptopFactory::
UpdateRecord(int customer_id, int order_num) {
	int size;

	// create an instance of the MapOp
	MapOp op;
	op.opcode = 1;
	op.arg1 = customer_id;
	op.arg2 = order_num;

	// obtain a lock, and move the op to the vector
	{
		std::unique_lock<std::mutex> ul(log_lock);
		smr_log->push_back(op);

		// update the last index with the
		int prev_last_idx = metadata->GetLastIndex();
		int prev_commited_idx = metadata->GetCommittedIndex();
		metadata->UpdateLastIndex(prev_last_idx + 1);

		// marshal the metadata(factoryid, committed_idx, last_idx, MapOp)
		char buffer[32];
		ReplicationRequest request(metadata, op);
		request.Marshal(buffer);
		size = request.Size();

		if (!stub.SendReplicationRequest(buffer, size)) {
			// TODO: error handling - have not received from one of the message from the backup

		}

		// update the commited index
		metadata->UpdateCommitedIndex(prev_commited_idx + 1);

		// update the record
		(*customer_record)[customer_id] = order_num;
	}
	return;
}

int LaptopFactory::
ReadRecord(int customer_id) {
	// obtain a lock, and return value if the key exists, otherwise return -1
	{
		std::unique_lock<std::mutex> ul(log_lock);
		auto it = customer_record->find(customer_id);
		if (it != customer_record->end()) { // found the key, return the value
			return (*customer_record)[customer_id];
		} else { // key not found, return -1
			return -1; 
		}
	}
}

/**
 * Entrance to the engineering thread
*/
void LaptopFactory::
EngineerThread(std::unique_ptr<ServerSocket> socket, 
				int engieer_id, 
				std::shared_ptr<std::map<int, int>> customer_record,
				std::shared_ptr<std::vector<MapOp>> smr_log,
				std::shared_ptr<ServerMetadata> metadata) {
	
	// set the private variable
	this->metadata = metadata;
	this->customer_record = customer_record;
	this->smr_log = smr_log;

	stub.Init(std::move(socket));

	int sender;
	while (true) {
		sender = stub.IdentifySender(); // identify sender; PrimaryServer or Customer
		switch (sender) {
			case PFA_IDENTIFIER:
				PfaHandler();
				break;
			case CUSTOMER_IDENTIFIER:
				CustomerHandler(engieer_id);
				break;
			default:
				std::cout << "Identifier Error!" << std::endl;
				break;
		}
	}
}

bool LaptopFactory::PfaHandler() {
	ReplicationRequest request;
	bool success;
	request = stub.ReceiveReplication();

	std::promise<bool> prom;
	std::future<bool> fut = prom.get_future();

	std::unique_ptr<IdleAdminRequest> idle_req = 
		std::unique_ptr<IdleAdminRequest>(new IdleAdminRequest);
	idle_req->repl_request = request;
	idle_req->repl_prom = std::move(prom);

	rep_lock.lock();
	req.push(std::move(idle_req));
	rep_cv.notify_one();
	rep_lock.unlock();

	success = fut.get();
	return success;
}

void LaptopFactory::CustomerHandler(int engineer_id) {
	std::unique_ptr<CustomerRecord> entry;
	CustomerRequest request;
	LaptopInfo laptop;
	int request_type, customer_id, order_num;
	int factory_id, primary_id;

	request = stub.ReceiveRequest();
	if (!request.IsValid()) {
		return;
	}
	request_type = request.GetRequestType();
	switch (request_type) {
		case UPDATE_REQUEST: // Update logic: PFA only
			if (!metadata->IsPrimary()) {
				stub.ConnectWithBackups(metadata);
			}

			laptop = CreateLaptop(request, engineer_id);
			stub.ShipLaptop(laptop);
			break;
		case READ_REQUEST: // Read logic: both PFA, IFA
			// read the record, and send the record
			laptop = GetLaptopInfo(request, engineer_id);
			customer_id = laptop.GetCustomerId();
			order_num = ReadRecord(customer_id);

			// get the record to return to the client
			entry = std::unique_ptr<CustomerRecord>(new CustomerRecord());
			entry->SetRecord(customer_id, order_num);

			stub.ReturnRecord(std::move(entry));
			stub.ShipLaptop(laptop);
			break;
		default:
			std::cout << "Undefined Request: "
				<< request_type << std::endl;
	}
}

void LaptopFactory::PrimaryAdminThread(int id) {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	int customer_id, order_num;
	while (true) {
		ul.lock();

		if (erq.empty()) {
			erq_cv.wait(ul, [this]{ return !erq.empty(); });
		}

		auto req = std::move(erq.front());
		erq.pop();

		// based on the request obtained from the erq
			// update the record with the MapOp
			// make sure to lock the access to the log

		ul.unlock();

		// get the customer_id and order_num from the request
		customer_id = req->laptop.GetCustomerId();
		order_num = req->laptop.GetOrderNumber();

		// update the record and set the adminid
		req->laptop.SetAdminId(id);
		req->prom.set_value(req->laptop);
		UpdateRecord(customer_id, order_num); 
	}
}

void LaptopFactory::IdleAdminThread(int id) {

	std::unique_lock<std::mutex> rl(rep_lock, std::defer_lock);
	int last_idx, committed_idx, primary_id;
	int customer_id, order_num;

	while (true) {
		rl.lock();

		if (req.empty()) {
			rep_cv.wait(rl, [this]{ return !req.empty(); });
		}

		auto request = std::move(req.front());
		req.pop();

		// based on the request obtained from the erq
			// update the record with the MapOp
			// make sure to lock the access to the log

		rl.unlock();

		// get the information
		last_idx = request->repl_request.GetLastIdx();
		committed_idx = request->repl_request.GetCommitedIdx();
		primary_id = request->repl_request.GetPrimaryId();
		customer_id = request->repl_request.GetArg1();
		order_num = request->repl_request.GetArg2();

		// update the metadata; commited index, last index
		metadata->SetPrimaryId(primary_id);
		metadata->UpdateCommitedIndex(committed_idx); // TODO: update accordingly
		metadata->UpdateLastIndex(last_idx);

		// add to the smr

		UpdateRecord(customer_id, order_num); 
	}
}