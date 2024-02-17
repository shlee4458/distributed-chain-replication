#ifndef __SERVERMETADATA_H__
#define __SERVERMETADATA_H__

#include <vector>
#include <memory>
#include <map>
#include <string.h>

#include "ClientSocket.h"

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
    int last_index;
    int committed_idx;
    int primary_id;
    int factory_id;
    bool is_primary = false;
    std::vector<std::shared_ptr<ServerNode>> neighbors;
    std::vector<std::shared_ptr<ClientSocket>> primary_sockets; // socket to the backup nodes as a primary
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;

public:
    ServerMetadata();

    int GetPrimaryId();
    int GetFactoryId();
    int GetLastIndex();
    int GetCommittedIndex();
    std::vector<std::shared_ptr<ServerNode>> GetNeighbors();
    std::vector<std::shared_ptr<ClientSocket>> GetPrimarySockets();
    MapOp GetOp(int idx);

    void SetFactoryId(int id);
    void SetPrimaryId(int id);
    void UpdateLastIndex(int idx);
    void UpdateCommitedIndex(int idx);
    void AppendLog(MapOp op);
    void UpdateRecord(int customer_id, int order_num);
    void ExecuteLog(int idx);
    int GetValue(int customer_id);

    bool WasBackup();
    bool IsPrimary();

    void AddNeighbors(std::shared_ptr<ServerNode> node);
    void InitNeighbors();
};

#endif