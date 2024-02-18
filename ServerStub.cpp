#include "ServerStub.h"

#include <iostream>
#include <deque>

#define PFA_IDENTIFIER 1

ServerStub::ServerStub() {}

void ServerStub::Init(std::shared_ptr<ServerSocket> socket) {
	this->socket = std::move(socket);
}

CustomerRequest ServerStub::ReceiveRequest() {
	char buffer[32];
	CustomerRequest request;
	if (socket->Recv(buffer, request.Size(), 0)) {
		request.Unmarshal(buffer);
	}
	return request;
}

int ServerStub::ShipLaptop(LaptopInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	return socket->Send(buffer, info.Size(), 0);
}

int ServerStub::ReturnRecord(std::shared_ptr<CustomerRecord> record) {
	char buffer[32];
	record->Marshal(buffer);
	return socket->Send(buffer, record->Size(), 0);
}

int ServerStub::SendReplicationRequest(char* buffer, int size, const std::deque<std::shared_ptr<ClientSocket>>& primary_sockets) {

	// iterate over all the neighbor nodes, and send the replication request
	int total_response = 0;
	char response_buffer[4];
	Identifier identifier;

	for (auto const& socket : primary_sockets) {

		// if any one of the idle servers failed
			// consider response was received
			// continue sending it to the other idle servers
		if (!socket->Send(buffer, size, 0)) {
			total_response++;
			continue;
		}

		if (socket->Recv(response_buffer, sizeof(identifier), 0)) {
			identifier.Unmarshal(response_buffer);
			total_response += identifier.GetIdentifier();
		}

	}
	return total_response;
}

int ServerStub::SendIdentifier(const std::deque<std::shared_ptr<ClientSocket>>& primary_sockets) const {

	if (primary_sockets.empty()) { // if no neighbors are present, do not send
		return 1;
	}

	// send identifier to the idle server
	char identifier_buffer[4];
	int identifier_size;
	auto identifier = std::shared_ptr<Identifier>(new Identifier());
	identifier->SetIdentifier(PFA_IDENTIFIER);
	identifier->Marshal(identifier_buffer); // store the identifier value in the buffer
	identifier_size = identifier->Size();

	for (auto const& socket : primary_sockets) {
		std::cout << "There is a peer" << std::endl;
		if (!socket->Send(identifier_buffer, identifier_size)) {
			return 0; // failed to send an identifier to an idle server
		}
	}
	return 1;
}

int ServerStub::IdentifySender() const {
	// return 1 if it is pfa, 2 if it is customer
	char buffer[4];
	auto identifier = std::shared_ptr<Identifier>(new Identifier());
	if (socket->Recv(buffer, sizeof(int), 0)) {
		identifier->Unmarshal(buffer);
		return identifier->GetIdentifier();
	}
	return 0; // identification failed
}

ReplicationRequest ServerStub::ReceiveReplication() const {
	char buffer[32];
	ReplicationRequest request;
	
	int size = request.Size();
	if (socket->Recv(buffer, size, 0)) {
		std::cout << "Replication Received!!!!" << std::endl;
		request.Unmarshal(buffer);
	}
	return request;
}


int ServerStub::RespondToPrimary() const {
	char buffer[4];
	Identifier identifier;
	int size = identifier.Size();
	identifier.SetIdentifier(PFA_IDENTIFIER);
	identifier.Marshal(buffer);
	return socket->Send(buffer, size, 0);
}