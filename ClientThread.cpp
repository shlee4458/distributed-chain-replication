#include "ClientThread.h"
#include "Messages.h"

#include <iostream>

#define UPDATE_REQUEST 1
#define READ_REQUEST 2
#define DEBUG 3
#define CLIENT_IDENTIFIER 2

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::
ThreadBody(std::string ip, int port, int customer_id, int num_requests, int request_type) {
	CustomerRequest request;
	LaptopInfo laptop;
	CustomerRecord record;
	Identifier identifier;

	this->customer_id = customer_id;
	this->num_requests = num_requests;
	this->request_type = request_type;

	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}
	for (int i = 0; i < num_requests; i++) {

		// send the identifier first
		identifier.SetIdentifier(CLIENT_IDENTIFIER);
		stub.SendIdentifier(identifier);

		timer.Start();
		// based on the request_type, call different RPC
		switch (request_type) {
			case UPDATE_REQUEST:
				request.SetRequest(customer_id, i, UPDATE_REQUEST);
				laptop = stub.Order(request);
				break;
			case READ_REQUEST:
			case DEBUG:
				// change the request record to 2
				request.SetRequest(i, -1, READ_REQUEST);
				record = stub.ReadRecord(request);
				if (request_type == DEBUG && record.IsValid()) {
					record.Print();
				}
				break;
			default:
				break;
		}
		timer.EndAndMerge();
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}

