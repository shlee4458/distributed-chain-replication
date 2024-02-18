#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>
#define DEBUG 1

ServerMetadata::ServerMetadata() 
: last_idx(-1), committed_idx(-1), primary_id(-1), factory_id(-1), neighbors() { }

int ServerMetadata::GetPrimaryId() {
    return primary_id;
}

int ServerMetadata::GetFactoryId() {
    return factory_id;
}

int ServerMetadata::GetCommittedIndex() {
    return committed_idx;
}

int ServerMetadata::GetLastIndex() {
    return last_idx;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

void ServerMetadata::SetPrimaryId(int id) {
	primary_id = id;
    return;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
    return;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    last_idx = idx;
    return;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    committed_idx = idx;
    return;
}

void ServerMetadata::AppendLog(MapOp op) {
    smr_log.push_back(op);
    last_idx++;
    return;
}

MapOp ServerMetadata::GetOp(int idx) {
    return smr_log[idx];
}

void ServerMetadata::ExecuteLog(int idx) {
    int customer_id, order_num;
    
    MapOp op = GetOp(idx);
    customer_id = op.arg1;
    order_num = op.arg2;

    customer_record[customer_id] = order_num;
    if (DEBUG) {
        std::cout << "Record Updated for client: " << customer_id 
            << " Order Num: " << order_num << std::endl;
    } 

    committed_idx++;
    return;
}

bool ServerMetadata::WasBackup() {
    return primary_id != -1;
}

bool ServerMetadata::IsPrimary() {
    return primary_id == factory_id;
}

int ServerMetadata::GetValue(int customer_id) {
    std::unique_lock<std::mutex> ml(meta_lock);
    auto it = customer_record.find(customer_id);
    if (it != customer_record.end()) { // found the key, return the value
        return customer_record[customer_id];
    } else { // key not found, return -1
        std::cout << "Key not found!" << std::endl;
        return -1; 
    }
}

int ServerMetadata::GetPeerSize() {
    return neighbors.size();
}

int ServerMetadata::GetNeighborSize() {
    return primary_sockets.size();
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node) {
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    // empty the primary sockets; primary -> idle -> primary
    primary_sockets.clear();

	std::string ip;
	int port;
	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            primary_sockets.push_back(std::move(socket));
        }
	}
}

std::deque<std::shared_ptr<ClientSocket>> ServerMetadata::GetPrimarySockets() {
    return primary_sockets;
}

ReplicationRequest ServerMetadata::GetReplicationRequest(MapOp op) {
    int op_code = op.opcode;
    int op_arg1 = op.arg1;
    int op_arg2 = op.arg2;
    return ReplicationRequest(last_idx, committed_idx, primary_id, op_code, op_arg1, op_arg2);
}

int ServerMetadata::SendReplicationRequest(MapOp op) {

	char buffer[32];
    int size;

    // get replication request object
	ReplicationRequest request = GetReplicationRequest(op);
	request.Marshal(buffer);
	size = request.Size();

	// iterate over all the neighbor nodes, and send the replication request
	int total_response = 0;
	char response_buffer[4];
	Identifier identifier;

    // synchronize sending the replication message
	for (auto const& socket : primary_sockets) {

		// if any one of the idle servers failed
			// consider response was received
			// continue sending it to the other idle servers
		if (!socket->Send(buffer, size, 0)) {
			total_response++;
			continue;
		}

		if (socket->Recv(response_buffer, sizeof(identifier), 0)) {
			identifier.Unmarshal(response_buffer);
			total_response += identifier.GetIdentifier();
		}
	}

    // corner case: less than initial peer count has joined the network in the initiation
    total_response += GetPeerSize() - GetNeighborSize();

    // debug
    std::cout << request << std::endl;

    // check if the message received matches the size of the neighbors
	if (total_response != GetNeighborSize()) {
        if (DEBUG) {
            std::cout << "Some neighbor has not updated the log, so I am not executing the log!" << std::endl;
        }
		return 0;
	}

    return 1;
}