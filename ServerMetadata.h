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

    bool WasBackup();
    bool IsPrimary();

    void AddNeighbors(std::unique_ptr<ServerNode> node);
    void ConnectWithNeighbors(std::vector<std::unique_ptr<ClientSocket>> primary_sockets);
};

#endif