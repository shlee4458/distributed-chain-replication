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
    std::unique_lock<std::mutex> ml(meta_lock);
    return committed_idx;
}

int ServerMetadata::GetLastIndex() {
    std::unique_lock<std::mutex> ml(meta_lock);
    return last_index;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

void ServerMetadata::SetPrimaryId(int id) {
    std::unique_lock<std::mutex> ml(meta_lock);
	primary_id = id;
    return;
}

void ServerMetadata::SetFactoryId(int id) {
    std::unique_lock<std::mutex> ml(meta_lock);
    factory_id = id;
    return;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    std::unique_lock<std::mutex> ml(meta_lock);
    last_index = idx;
    return;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    std::unique_lock<std::mutex> ml(meta_lock);
    committed_idx = idx;
    return;
}

void ServerMetadata::AppendLog(MapOp op) {
    std::unique_lock<std::mutex> ml(meta_lock);
    smr_log.push_back(op);
    last_index++;
    return;
}

MapOp ServerMetadata::GetOp(int idx) {
    return smr_log[idx];
}

void ServerMetadata::ExecuteLog(int idx) {
    std::unique_lock<std::mutex> ml(meta_lock);
    MapOp op = GetOp(idx);
    UpdateRecord(op.arg1, op.arg2);
    committed_idx++;
    return;
}

void ServerMetadata::UpdateRecord(int customer_id, int order_num) {
    customer_record[customer_id] = order_num;
    std::cout << "Record Updated for client: " << customer_id 
              << " Order Num: " << order_num << std::endl;
    return;
}

bool ServerMetadata::WasBackup() {
    return primary_id != -1;
}

bool ServerMetadata::IsPrimary() {
    std::unique_lock<std::mutex> ml(meta_lock);
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

int ServerMetadata::GetNeighborSize() {
    return GetPrimarySockets().size();
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