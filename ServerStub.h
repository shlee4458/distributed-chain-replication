#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>

#include "ServerSocket.h"
#include "ClientSocket.h"
#include "Messages.h"
#include "ServerMetadata.h"

class ServerStub {
private:
	std::unique_ptr<ServerSocket> socket;
	std::vector<std::unique_ptr<ClientSocket>> primary_sockets; // socket to the backup nodes as a primary
	std::unique_ptr<ServerSocket> backup_socket; // socket to the primary node as a backup
public:
	ServerStub();
	void Init(std::unique_ptr<ServerSocket> socket);
	CustomerRequest ReceiveRequest();
	int ShipLaptop(LaptopInfo info);
	int ReturnRecord(std::unique_ptr<CustomerRecord> record);
	void ConnectWithBackups(std::shared_ptr<ServerMetadata> metadata);
	int SendReplicationRequest(char* buffer, int size);
	ReplicationRequest ReceiveReplication();
	int IdentifySender();
};

#endif // end of #ifndef __SERVER_STUB_H__
