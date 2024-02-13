#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <map>

#include "ServerSocket.h"
#include "ServerThread.h"

int main(int argc, char *argv[]) {
	int port;
	int engineer_cnt = 0;
	int num_experts;
	ServerSocket socket;
	LaptopFactory factory;
	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;

	auto customer_record = std::make_shared<std::map<int, int>>();
	auto smr_log = std::make_shared<std::vector<MapOp>>();
	
	if (argc < 3) {
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << "[port #] [# experts]" << std::endl;
		return 0;
	}
	port = atoi(argv[1]);
	num_experts = atoi(argv[2]);

	for (int i = 0; i < num_experts; i++) {
		std::thread expert_thread(&LaptopFactory::AdminThread, 
				&factory, engineer_cnt++);
		thread_vector.push_back(std::move(expert_thread));
	}

	if (!socket.Init(port)) {
		std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	while ((new_socket = socket.Accept())) {
		std::thread engineer_thread(&LaptopFactory::EngineerThread, 
				&factory, std::move(new_socket), 
				engineer_cnt++, customer_record, smr_log);
		thread_vector.push_back(std::move(engineer_thread));
	}
	return 0;
}
