#include "ServerStub.h"
#include "ServerMetadata.h"

ServerStub::ServerStub() {}

void ServerStub::Init(std::unique_ptr<ServerSocket> socket, std::shared_ptr<ServerMetadata> metadata) {
	this->socket = std::move(socket);
	this->metadata = metadata;
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
void ServerStub::ConnectWithBackups() {

	// if there is a socket connected with the primary server, close the connection
	if (metadata->WasBackup()) {
		backup_socket->Close();
	}

	// populate sockets vector by connecting with the neighbors
	metadata->ConnectWithNeighbors(primary_sockets);
}

void ServerStub::SendReplicationRequest() {
	// iterate over all the neighbor nodes, and send the replication request


}

