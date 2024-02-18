#ifndef __SERVERMETADATA_H__
#define __SERVERMETADATA_H__

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <string.h>
#include <deque>

#include "ClientSocket.h"
#include "Messages.h"

struct ServerNode {
    int id;
    std::string ip;
    int port;
};

struct MapOp {
	int opcode; // operation code : 1 - update value
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class ServerMetadata {
private:
    int last_idx;
    int committed_idx;
    int primary_id;
    int factory_id;
    bool is_primary = false;
    std::vector<std::shared_ptr<ServerNode>> neighbors;
    std::deque<std::shared_ptr<ClientSocket>> primary_sockets; // socket to the backup nodes as a primary
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;
    std::mutex meta_lock;

public:
    ServerMetadata();

    int GetPrimaryId();
    int GetFactoryId();
    int GetLastIndex();
    int GetCommittedIndex();
    int GetNeighborSize();
    int GetPeerSize();
    std::vector<std::shared_ptr<ServerNode>> GetNeighbors();
    std::deque<std::shared_ptr<ClientSocket>> GetPrimarySockets();
    MapOp GetOp(int idx);
    ReplicationRequest GetReplicationRequest(MapOp op);

    void SetFactoryId(int id);
    void SetPrimaryId(int id);
    void UpdateLastIndex(int idx);
    void UpdateCommitedIndex(int idx);
    void AppendLog(MapOp op);
    void ExecuteLog(int idx);
    int GetValue(int customer_id);

    bool WasBackup();
    bool IsPrimary();

    void AddNeighbors(std::shared_ptr<ServerNode> node);
    void InitNeighbors();
    int SendReplicationRequest(MapOp op);
};

#endif