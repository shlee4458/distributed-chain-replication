#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>

#include "ServerSocket.h"
#include "ClientSocket.h"
#include "Messages.h"
#include "ServerMetadata.h"

class ServerStub {
private:
	std::shared_ptr<ServerSocket> socket;
	
public:
	ServerStub();

	void Init(std::shared_ptr<ServerSocket> socket);

	CustomerRequest ReceiveRequest();
	int ShipLaptop(LaptopInfo info);
	int ReturnRecord(std::shared_ptr<CustomerRecord> record);
	void ConnectWithBackups(std::shared_ptr<ServerMetadata> metadata);
	int SendReplicationRequest(char* buffer, int size, const std::deque<std::shared_ptr<ClientSocket>>& primary_sockets);

	ReplicationRequest ReceiveReplication() const;
	int IdentifySender() const;
	int SendIdentifier(const std::deque<std::shared_ptr<ClientSocket>>& primary_sockets) const;
	int RespondToPrimary() const;
};

#endif // end of #ifndef __SERVER_STUB_H__
