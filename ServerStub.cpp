#include "ServerStub.h"

#define PFA_IDENTIFIER 1

ServerStub::ServerStub() {}

void ServerStub::Init(std::unique_ptr<ServerSocket> socket) {
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

int ServerStub::ReturnRecord(std::unique_ptr<CustomerRecord> record) {
	char buffer[32];
	record->Marshal(buffer);
	return socket->Send(buffer, record->Size(), 0);
}

// establish connection with the backup servers
	// open connection with all the neighboring servers as the client
void ServerStub::ConnectWithBackups(std::shared_ptr<ServerMetadata> metadata) {
	metadata->InitNeighbors(primary_sockets);
}

int ServerStub::SendReplicationRequest(char* buffer, int size) {

	// iterate over all the neighbor nodes, and send the replication request
	for (auto const& socket : primary_sockets) {
		if (!socket->Send(buffer, size, 0)) { // idle server failure
			return 0;
		}
	}
	return 1; // upon successfully receiving all the messages, return 1
}

int ServerStub::SendIdentifier() {

	// send identifier to the idle server
	char identifier_buffer[4];
	int identifier_size;
	std::unique_ptr<Identifier> identifier;
	identifier->SetIdentifier(PFA_IDENTIFIER);
	identifier->Marshal(identifier_buffer); // store the identifier value in the buffer
	identifier_size = identifier->Size();

	for (auto const& socket : primary_sockets) {
		if (socket->Send(identifier_buffer, identifier_size)) {
			return 0; // failed to send an identifier to an idle server
		}
	}
	return 1;
}

int ServerStub::IdentifySender() {
	// return 1 if it is pfa, 2 if it is customer
	char buffer[4];
	std::unique_ptr<Identifier> identifier;
	if (socket->Recv(buffer, sizeof(int), 0)) {
		identifier->Unmarshal(buffer);
		return identifier->GetIdentifier();
	}
	return 0; // identification failed
}

ReplicationRequest ServerStub::ReceiveReplication() {
	char buffer[32];
	ReplicationRequest request;
	int size = request.Size();
	if (socket->Recv(buffer, size, 0)) {
		request.Unmarshal(buffer);
	}
	return request;
}