#include <memory>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

#include "ServerMetadata.h"
#include "ClientSocket.h"
#include "ServerThread.h"

ServerMetadata::ServerMetadata() 
: last_index(-1), committed_index(-1), primary_id(-1), factory_id(), neighbors() { }

int ServerMetadata::GetPrimaryId() {
    return primary_id;
}

int ServerMetadata::GetFactoryId() {
    return factory_id;
}

int ServerMetadata::GetCommittedIndex() {
    return committed_index;
}

int ServerMetadata::GetLastIndex() {
    return last_index;
}

std::vector<std::unique_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    last_index = idx;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    committed_index = idx;
}

bool ServerMetadata::WasBackup() {
    return primary_id != -1;
}

bool ServerMetadata::IsPrimary() {
    return primary_id == factory_id;
}

void ServerMetadata::AddNeighbors(std::unique_ptr<ServerNode> node) {
    neighbors.push_back(std::move(node));
}

void ServerMetadata::ConnectWithNeighbors(std::vector<std::unique_ptr<ClientSocket>> primary_sockets) {
    // update the vector with ServerNode information
	std::string ip;
	int port;
	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		auto socket = std::unique_ptr<ClientSocket>();
		socket->Init(ip, port);
		primary_sockets.push_back(std::move(socket));
	}
    return;
}

int ServerMetadata::Marshal(char *buffer, MapOp op) {
	int net_customer_id = htonl(factory_id);
	int net_last_index = htonl(last_index);
    int net_committed_index = htonl(committed_index);
    int net_opcode = htonl(op.opcode);
    int net_arg1 = htonl(op.arg1);
    int net_arg2 = htonl(op.arg2);

	int offset = 0;
	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_last_index, sizeof(net_last_index));
	offset += sizeof(net_last_index);
	memcpy(buffer + offset, &net_committed_index, sizeof(net_committed_index));
	offset += sizeof(net_committed_index);
	memcpy(buffer + offset, &net_opcode, sizeof(net_opcode));
	offset += sizeof(net_opcode);
	memcpy(buffer + offset, &net_arg1, sizeof(net_arg1));
    offset += sizeof(net_arg1);
	memcpy(buffer + offset, &net_arg2, sizeof(net_arg2));
    return sizeof(int) * 6;
}
