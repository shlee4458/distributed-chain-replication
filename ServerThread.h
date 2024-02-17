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
#include "ServerMetadata.h"

struct PrimaryAdminRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
	int stub_idx;
};

struct IdleAdminRequest {
	ReplicationRequest repl_request;
	std::promise<bool> repl_prom;
	int stub_idx;
};

class LaptopFactory {
private:
	std::queue<std::shared_ptr<PrimaryAdminRequest>> erq;
	std::queue<std::shared_ptr<IdleAdminRequest>> req;

	std::mutex erq_lock;
	std::mutex log_lock;
	std::mutex rep_lock;

	std::condition_variable erq_cv;
	std::condition_variable rep_cv;

	std::shared_ptr<std::map<int, int>> customer_record;
	std::shared_ptr<std::vector<MapOp>> smr_log;
	std::shared_ptr<ServerMetadata> metadata;

	std::vector<ServerStub> stubs;

	LaptopInfo GetLaptopInfo(CustomerRequest order, int engineer_id);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id, int stub_idx);
	bool PfaHandler(int stub_idx);
	void CustomerHandler(int engineer_id, int stub_idx);

	int ReadRecord(int customer_id);
	void PrimaryMaintainLog(int customer_id, int order_num, int stub_idx);
	void IdleMaintainLog(int customer_id, int order_num, int req_last, int req_committed, bool was_primary);

	void ExecuteLog(int idx);

public:
	void EngineerThread(std::unique_ptr<ServerSocket> socket, 
						int engieer_id, 
						std::shared_ptr<ServerMetadata> metadata);
	void PrimaryAdminThread(int id);
	void IdleAdminThread(int id);
};

#endif // end of #ifndef __SERVERTHREAD_H__

