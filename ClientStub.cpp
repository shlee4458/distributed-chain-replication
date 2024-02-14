#include "ClientStub.h"

ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);	
}

LaptopInfo ClientStub::Order(CustomerRequest request) {
	LaptopInfo info;
	char buffer[32];
	int size;
	request.Marshal(buffer);
	size = request.Size();
	if (socket.Send(buffer, size, 0)) {
		size = info.Size();
		if (socket.Recv(buffer, size, 0)) {
			info.Unmarshal(buffer);
		} 
	}
	return info;
}

CustomerRecord ClientStub::ReadRecord(CustomerRequest request) {
	LaptopInfo info;
	CustomerRecord record;
	char record_buffer[32], info_buffer[32];
	int record_size, info_size;
	request.Marshal(record_buffer);
	record_size = request.Size();
	if (socket.Send(record_buffer, record_size, 0)) {

		// recv the record
		record_size = record.Size();
		if (socket.Recv(record_buffer, record_size, 0)) {
			record.Unmarshal(record_buffer);
		}

		// recv the info
		info_size = info.Size();
		if (socket.Recv(info_buffer, info_size, 0)) {
			info.Unmarshal(info_buffer);
		}
	}
	return record;
}

int ClientStub::SendIdentifier(Identifier identifier) {
	char buffer[4];
	int size;
	identifier.Marshal(buffer);
	return socket.Send(buffer, identifier.Size(), 0);
}
