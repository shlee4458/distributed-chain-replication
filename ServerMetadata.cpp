#include "ServerMetadata.h"

#include <string.h>
#include <iostream>

ServerMetadata::ServerMetadata() 
: last_index(-1), committed_idx(-1), primary_id(-1), factory_id(-1), neighbors() { }

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
    return last_index;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

void ServerMetadata::SetPrimaryId(int id) {
	primary_id = id;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    last_index = idx;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    committed_idx = idx;
}

void ServerMetadata::AppendLog(MapOp op) {
    smr_log.push_back(op);
    last_index++;
}

MapOp ServerMetadata::GetOp(int idx) {
    return smr_log[idx];
}

void ServerMetadata::ExecuteLog(int idx) {
    MapOp op = GetOp(idx);
    UpdateRecord(op.arg1, op.arg2);
    committed_idx++;
}

void ServerMetadata::UpdateRecord(int customer_id, int order_num) {
    customer_record[customer_id] = order_num;
    std::cout << "Record Updated for client: " << customer_id 
              << " Order Num: " << order_num << std::endl;
}

bool ServerMetadata::WasBackup() {
    return primary_id != -1;
}

bool ServerMetadata::IsPrimary() {
    return primary_id == factory_id;
}

int ServerMetadata::GetValue(int customer_id) {
    auto it = customer_record.find(customer_id);
    if (it != customer_record.end()) { // found the key, return the value
        return customer_record[customer_id];
    } else { // key not found, return -1
        std::cout << "Key not found!" << std::endl;
        return -1; 
    }
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node) {
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    // empty the primary sockets; primary -> idle -> primary
    primary_sockets.clear();

    // update the vector with ServerNode information
	std::string ip;
    // std::shared_ptr<ClientSocket> socket;
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

std::vector<std::shared_ptr<ClientSocket>> ServerMetadata::GetPrimarySockets() {
    return primary_sockets;
}