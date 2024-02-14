#include "ServerStub.h"
#include "ServerMetadata.h"

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

	// if there is a socket connected with the primary server, close the connection
	if (metadata->WasBackup()) {
		backup_socket->Close();
	}

	// populate sockets vector by connecting with the neighbors
	// metadata->ConnectWithNeighbors(primary_sockets);
}

int ServerStub::SendReplicationRequest(char* buffer, int size) {

	// send identifier to the idle server first
	char identifier_buffer[4];
	int identifier_size;
	Identifier identifier;
	identifier.SetIdentifier(PFA_IDENTIFIER);
	identifier.Marshal(identifier_buffer); // store the identifier value in the buffer
	identifier_size = identifier.Size();

	// iterate over all the neighbor nodes, and send the replication request
	char buffer[32];
	for (auto const& socket : primary_sockets) {
		socket->Send(identifier_buffer, identifier_size); // first send the identifier
		if (socket->Send(buffer, size, 0)) {
			// receive the response from the backup server; expect htonl of single 4 byte int
			// if message not received exit with 0
			if (!socket->Recv(buffer, sizeof(int), 0)) {
				return 0;
			}
		}
	}
	return 1; // upon successfully receiving all the messages, return 1
}

int ServerStub::IdentifySender() {
	// return 1 if it is pfa, 2 if it is customer
	char buffer[4];
	Identifier identifier;
	if (socket->Recv(buffer, sizeof(int), 0)) {
		identifier.Unmarshal(buffer);
		return identifier.GetIdentifier();
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