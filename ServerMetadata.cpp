#include <memory>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

#include "ServerMetadata.h"
#include "ClientSocket.h"
#include "ServerThread.h"
#include "Messages.h"

ServerMetadata::ServerMetadata() 
: last_index(-1), committed_idx(-1), primary_id(-1), factory_id(), neighbors() { }

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

std::vector<std::unique_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
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