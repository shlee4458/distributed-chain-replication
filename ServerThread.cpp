#include <iostream>
#include <memory>
#include <map>

#include "ServerThread.h"

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
CreateLaptop(CustomerRequest request, int engineer_id, int stub_idx) {
	LaptopInfo laptop;
	laptop.CopyRequest(request);
	laptop.SetEngineerId(engineer_id);

	std::promise<LaptopInfo> prom;
	std::future<LaptopInfo> fut = prom.get_future();

	std::shared_ptr<PrimaryAdminRequest> req = 
		std::shared_ptr<PrimaryAdminRequest>(new PrimaryAdminRequest);
	req->laptop = laptop;
	req->prom = std::move(prom);
	req->stub_idx = stub_idx;

	erq_lock.lock();
	erq.push(std::move(req));
	erq_cv.notify_one();
	erq_lock.unlock();

	laptop = fut.get();
	return laptop;
}

void LaptopFactory::
PrimaryMaintainLog(int customer_id, int order_num, int stub_idx) {

	int size;
	int neighbor_size;
	int response_received;
	MapOp op;
	op.opcode = 1;
	op.arg1 = customer_id;
	op.arg2 = order_num;

	// obtain a lock, and move the op to the vector
	{
		// if it changed to primary from idle server
		std::unique_lock<std::mutex> ul(log_lock);
		int prev_last_idx = metadata->GetLastIndex();
		int prev_commited_idx = metadata->GetCommittedIndex();

		// if it was previously an idle server, execute the last log
		if (prev_last_idx != prev_commited_idx) {
			metadata->ExecuteLog(prev_last_idx);
			std::cout << "Executed Log" << std::endl;
		}

		metadata->AppendLog(op);

		// marshal the metadata(factoryid, committed_idx, last_idx, MapOp)
		char buffer[32];
		ReplicationRequest request(metadata, op);
		request.Marshal(buffer);
		size = request.Size();

		{
			std::unique_lock<std::mutex> sl(stub_lock);
			neighbor_size = metadata->GetNeighborSize();
			response_received = stubs[stub_idx].SendReplicationRequest(buffer, size, metadata->GetPrimarySockets());
			if (response_received != neighbor_size) {
				std::cout << "Some neighbor has not updated the log, so I am not executing the log!" << std::endl;
				return;
			}
		}
		
		std::cout << request << std::endl;

		// execute log at the last index, and update the committed_index
		metadata->ExecuteLog(metadata->GetLastIndex());
		std::cout << "This was executed" << std::endl;
	}
	return;
}

void LaptopFactory::
IdleMaintainLog(int customer_id, int order_num, int req_last, int req_committed, bool was_primary) {
	MapOp op;
	op.opcode = 1;
	op.arg1 = customer_id;
	op.arg2 = order_num;
	{
		std::unique_lock<std::mutex> ul(log_lock);

		// append the new log and update the last_index
		metadata->AppendLog(op);

		// execute the log at the req.committed index
		// it is not the case that the server was primary
		if (req_committed >= 0 && !was_primary) { // need at least 1 MapOp to be present
			metadata->ExecuteLog(req_committed);
		}

	}
}


int LaptopFactory::
ReadRecord(int customer_id) {
	// obtain a lock, and return value if the key exists, otherwise return -1
	{
		std::unique_lock<std::mutex> ul(log_lock);
		return metadata->GetValue(customer_id);

	}
}

/**
 * Entrance to the engineering thread
*/
void LaptopFactory::
EngineerThread(std::unique_ptr<ServerSocket> socket, 
				int engieer_id, 
				std::shared_ptr<ServerMetadata> metadata) {
	
	// synchronize stub creation
	int stub_idx;
	this->metadata = metadata;
	{
		std::unique_lock<std::mutex> sl(stub_lock);
		ServerStub stub;
		stub.Init(std::move(socket));
		stubs.push_back(std::move(stub));
		stub_idx = stubs.size() - 1;
	}

	// if the current was primary, close the socket from primary to idle
	if (!metadata->IsPrimary()) {
		metadata->GetNeighbors().clear();
	}

	int sender = stubs[stub_idx].IdentifySender(); // identify sender; PrimaryServer or Customer
	while (true) {
		// sender = stub.IdentifySender(); // identify sender; PrimaryServer or Customer
		switch (sender) {
			case PFA_IDENTIFIER:
				std::cout << "I have received a message from the Primary server!" << std::endl;
				PfaHandler(stub_idx);
				std::cout << "CONNECTION WITH THE SERVER HAS BEEN TERMINATED" << std::endl;
				return;
				break;
			case CUSTOMER_IDENTIFIER:
				std::cout << "I have received a message from a customer!" << std::endl;
				CustomerHandler(engieer_id, stub_idx);
				std::cout << "CONNECTION WITH THE CLIENT HAS BEEN TERMINATED" << std::endl;
				return;
				break;
			default:
				return;
				break;
		}
	}
}

bool LaptopFactory::PfaHandler(int stub_idx) {

	ReplicationRequest request;
	// bool success;
	while (true) {
		request = stubs[stub_idx].ReceiveReplication();
		
		// Primary Failure: set the primary_id to -1
		if (!request.IsValid()) {
			metadata->SetPrimaryId(-1);
			std::cout << "Primary Server went down, gracefully exiting" << std::endl;
			return 0;
		}

		std::promise<bool> prom;
		std::future<bool> fut = prom.get_future();

		std::shared_ptr<IdleAdminRequest> idle_req = 
			std::shared_ptr<IdleAdminRequest>(new IdleAdminRequest);
		idle_req->repl_request = request;
		idle_req->repl_prom = std::move(prom);
		idle_req->stub_idx = stub_idx;

		rep_lock.lock();
		req.push(std::move(idle_req));
		rep_cv.notify_one();
		rep_lock.unlock();

		// success = fut.get();
		// return success;
	}
}

void LaptopFactory::CustomerHandler(int engineer_id, int stub_idx) {
	std::shared_ptr<CustomerRecord> entry;
	CustomerRequest request;
	LaptopInfo laptop;
	int request_type, customer_id, order_num;

	while (true) {
		
		request = stubs[stub_idx].ReceiveRequest();
		if (!request.IsValid()) {
			return;
		}
		request_type = request.GetRequestType();
		switch (request_type) {
			case UPDATE_REQUEST: // Update logic: PFA only
				if (!metadata->IsPrimary()) { // Idle -> Primary
					metadata->InitNeighbors();
					metadata->SetPrimaryId(metadata->GetFactoryId()); // set itself as the primary
					std::cout << "I wasn't primary! Priamry Id updated!!" << std::endl;	
					stubs[stub_idx].SendIdentifier(metadata->GetPrimarySockets()); // send one time identifier
				}
				laptop = CreateLaptop(request, engineer_id, stub_idx);
				stubs[stub_idx].ShipLaptop(laptop);
				break;
			case READ_REQUEST: // Read logic: both PFA, IFA
				// read the record, and send the record
				laptop = GetLaptopInfo(request, engineer_id);
				customer_id = laptop.GetCustomerId();
				std::cout << "Received a READ REQUEST for: " << customer_id << std::endl;
				order_num = ReadRecord(customer_id);

				// get the record to return to the client
				entry = std::shared_ptr<CustomerRecord>(new CustomerRecord());
				entry->SetRecord(customer_id, order_num);
				entry->Print();

				stubs[stub_idx].ReturnRecord(std::move(entry));
				// stub.ShipLaptop(laptop);
				break;
			default:
				std::cout << "Undefined Request: "
					<< request_type << std::endl;
		}
	}
}

void LaptopFactory::PrimaryAdminThread(int id) {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	int customer_id, order_num, stub_idx;
	while (true) {
		ul.lock();

		if (erq.empty()) {
			erq_cv.wait(ul, [this]{ return !erq.empty(); });
		}

		auto req = std::move(erq.front());
		erq.pop();
		ul.unlock();

		// get the customer_id and order_num from the request
		customer_id = req->laptop.GetCustomerId();
		order_num = req->laptop.GetOrderNumber();
		stub_idx = req->stub_idx;
		
		// update the record and set the adminid
		req->laptop.SetAdminId(id);
		req->prom.set_value(req->laptop);
		PrimaryMaintainLog(customer_id, order_num, stub_idx); 
	}
}

void LaptopFactory::IdleAdminThread(int id) {

	std::unique_lock<std::mutex> rl(rep_lock, std::defer_lock);
	int last_idx, committed_idx, primary_id;
	int customer_id, order_num, stub_idx;

	while (true) {
		rl.lock();

		if (req.empty()) {
			rep_cv.wait(rl, [this]{ return !req.empty(); });
		}
		std::cout << "Successfully received the replication request" << std::endl;

		auto request = std::move(req.front());
		req.pop();
		rl.unlock();

		// get the information
		last_idx = request->repl_request.GetLastIdx();
		committed_idx = request->repl_request.GetCommitedIdx();
		primary_id = request->repl_request.GetPrimaryId();
		customer_id = request->repl_request.GetArg1();
		order_num = request->repl_request.GetArg2();
		stub_idx = request->stub_idx;
		std::cout << "last_idx: " << last_idx << std::endl;
		std::cout << "committed_idx: " << committed_idx << std::endl;
		std::cout << "primary_id: " << primary_id << std::endl;
		std::cout << "customer_id: " << customer_id << std::endl;
		std::cout << "order_num: " << order_num << std::endl;

		// check if the current server was the primary
		bool was_primary = metadata->IsPrimary();

		// update the metadata; commited index, last index
		if (was_primary) {
			metadata->SetPrimaryId(primary_id);
			std::cout << "I have set the primary id to: " << primary_id << std::endl;
		}
		IdleMaintainLog(customer_id, order_num, last_idx, committed_idx, was_primary);

		// send to the primary that the log update is complete
		stubs[stub_idx].RespondToPrimary();
		std::cout << "I have responded to the primary!" << std::endl;
		request->repl_prom.set_value(true);
	}
}