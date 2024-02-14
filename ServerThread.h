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
#include "ServerStub.h"
#include "ServerSocket.h"

struct PrimaryAdminRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
};

struct IdleAdminRequest {
	ReplicationRequest repl_request;
	std::promise<bool> repl_prom;
};

struct MapOp {
	int opcode; // operation code : 1 - update value
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class LaptopFactory {
private:
	ServerStub stub;
	std::queue<std::unique_ptr<PrimaryAdminRequest>> erq;
	std::queue<std::unique_ptr<IdleAdminRequest>> req;

	std::mutex erq_lock;
	std::mutex log_lock;
	std::mutex rep_lock;

	std::condition_variable erq_cv;
	std::condition_variable rep_cv;
	std::shared_ptr<std::map<int, int>> customer_record;
	std::shared_ptr<std::vector<MapOp>> smr_log;
	std::shared_ptr<ServerMetadata> metadata;

	LaptopInfo GetLaptopInfo(CustomerRequest order, int engineer_id);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id);
	bool PfaHandler();
	void CustomerHandler(int engineer_id);

	int ReadRecord(int customer_id);
	void UpdateRecord(int customer_id, int order_num);
public:
	void EngineerThread(std::unique_ptr<ServerSocket> socket, 
						int engieer_id, 
						std::shared_ptr<std::map<int, int>> record,
						std::shared_ptr<std::vector<MapOp>> smr,
						std::shared_ptr<ServerMetadata> metadata);
	void PrimaryAdminThread(int id);
	void IdleAdminThread(int id);
};

#endif // end of #ifndef __SERVERTHREAD_H__

