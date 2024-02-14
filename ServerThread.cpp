#include <iostream>
#include <memory>
#include <map>

#include "ServerThread.h"
#include "ServerStub.h"

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

	std::unique_ptr<AdminRequest> req = 
		std::unique_ptr<AdminRequest>(new AdminRequest);
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
		int size = metadata->Marshal(buffer, op);
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

void LaptopFactory::
EngineerThread(std::unique_ptr<ServerSocket> socket, 
				int id, 
				std::shared_ptr<std::map<int, int>> record,
				std::shared_ptr<std::vector<MapOp>> smr,
				std::shared_ptr<ServerMetadata> metadata) {

	// set the private variable
	this->metadata = metadata;
	customer_record = record;
	smr_log = smr;

	int engineer_id = id;
	int request_type, customer_id, order_num;
	int factory_id, primary_id;

	CustomerRequest request;
	std::unique_ptr<CustomerRecord> entry;
	LaptopInfo laptop;

	stub.Init(std::move(socket), metadata);

	while (true) {
		request = stub.ReceiveRequest();
		if (!request.IsValid()) {
			break;	
		}
		request_type = request.GetRequestType();
		switch (request_type) {
			case 1: // PFA logic (admin thread)
				// if the current node is not the primary node, 
					// set the current node as the primary
					// and broadcast to the neighbor nodes
				if (!metadata->IsPrimary()) {
					stub.ConnectWithBackups();
				}

				laptop = CreateLaptop(request, engineer_id);
				stub.ShipLaptop(laptop);
				break;
			case 2: // IFA logic (engineer thread)
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
				std::cout << "Undefined laptop type: "
					<< request_type << std::endl;
		}
	}
}

void LaptopFactory::AdminThread(int id) {
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





// void LaptopFactory::AdminThread(int id) {
// 	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
// 	while (true) {
// 		ul.lock();

// 		if (erq.empty()) {
// 			erq_cv.wait(ul, [this]{ return !erq.empty(); });
// 		}

// 		auto req = std::move(erq.front());
// 		erq.pop();
// 		ul.unlock();

// 		std::this_thread::sleep_for(std::chrono::microseconds(100));
// 		req->laptop.SetAdminId(id);
// 		req->prom.set_value(req->laptop);	
// 	}
// }