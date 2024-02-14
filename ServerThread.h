#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <map>

#include "Messages.h"
#include "ServerSocket.h"

struct AdminRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
};

struct MapOp {
	int opcode; // operation code : 1 - update value
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class LaptopFactory {
private:
	ServerStub stub;
	std::queue<std::unique_ptr<AdminRequest>> erq;
	std::mutex erq_lock;
	std::mutex log_lock;
	std::condition_variable erq_cv;
	std::shared_ptr<std::map<int, int>> customer_record;
	std::shared_ptr<std::vector<MapOp>> smr_log;
	std::shared_ptr<ServerMetadata> metadata;

	LaptopInfo GetLaptopInfo(CustomerRequest order, int engineer_id);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id);

	int ReadRecord(int customer_id);
	void PrimaryUpdateRecord(int customer_id, int order_num);
public:
	void EngineerThread(std::unique_ptr<ServerSocket> socket, 
						int id, 
						std::shared_ptr<std::map<int, int>> record,
						std::shared_ptr<std::vector<MapOp>> smr,
						std::shared_ptr<ServerMetadata> metadata);
	void PrimaryAdminThread(int id);
	void IdleAdminThread(int id);
};

#endif // end of #ifndef __SERVERTHREAD_H__

