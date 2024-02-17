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
	
public:
	ServerStub();
	void Init(std::unique_ptr<ServerSocket> socket);
	CustomerRequest ReceiveRequest();
	int ShipLaptop(LaptopInfo info);
	int ReturnRecord(std::shared_ptr<CustomerRecord> record);
	void ConnectWithBackups(std::shared_ptr<ServerMetadata> metadata);
	int SendReplicationRequest(char* buffer, int size, std::vector<std::shared_ptr<ClientSocket>> primary_sockets);

	ReplicationRequest ReceiveReplication();
	int IdentifySender();
	int SendIdentifier(std::vector<std::shared_ptr<ClientSocket>> primary_sockets);
};

#endif // end of #ifndef __SERVER_STUB_H__
