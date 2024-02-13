#include "ClientThread.h"
#include "Messages.h"

#include <iostream>

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::
ThreadBody(std::string ip, int port, int id, int requests, int type) {
	customer_id = id;
	num_requests = requests;
	request_type = type;
	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}
	for (int i = 0; i < num_requests; i++) {
		CustomerRequest request;
		LaptopInfo laptop;
		CustomerRecord record;
		timer.Start();

		// based on the request_type, call different RPC
		switch (request_type) {
			case 1:
				request.SetRequest(customer_id, i, request_type);
				laptop = stub.Order(request);
				break;
			case 2:
			case 3:
				// change the request record to 2
				request.SetRequest(i, -1, 2);
				record = stub.ReadRecord(request);
				if (request_type == 3) {
					record.Print();
				}
			default:
				break;
		}

		timer.EndAndMerge();



		// if (!laptop.IsValid()) {
		// 	std::cout << "Invalid laptop " << customer_id << std::endl;
		// 	break;	
		// } 
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}

