#ifndef __SERVER_Metadata_H__
#define __SERVER_Metadata_H__

#include <vector>
#include <memory>
#include <string.h>

#include "ClientSocket.h"
#include "ServerThread.h"

struct ServerNode {
    int id;
    std::string ip;
    int port;
};

class ServerMetadata {
private:
    int last_index;
    int committed_idx;
    int primary_id;
    int factory_id;
    std::vector<std::unique_ptr<ServerNode>> neighbors;
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;

public:
    ServerMetadata();

    int GetPrimaryId();
    int GetFactoryId();
    int GetLastIndex();
    int GetCommittedIndex();
    std::vector<std::unique_ptr<ServerNode>> GetNeighbors();

    void SetFactoryId(int id);
    void SetPrimaryId(int id);
    void UpdateLastIndex(int idx);
    void UpdateCommitedIndex(int idx);
    void AppendLog(MapOp op);
    MapOp GetOp(int idx);
    void UpdateRecord(int customer_id, int order_num);
    void ExecuteLog(int idx);

    bool WasBackup();
    bool IsPrimary();

    void AddNeighbors(std::unique_ptr<ServerNode> node);
    void ConnectWithNeighbors(std::vector<std::unique_ptr<ClientSocket>> primary_sockets);
};

#endif