#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>

class CustomerRequest {
private:
	int customer_id;
	int order_number;
	int request_type;

public:
	CustomerRequest();
	void operator = (const CustomerRequest &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	void SetRequest(int cid, int order_num, int type);
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class LaptopInfo {
private:
	int customer_id;
	int order_number;
	int request_type;
	int engineer_id;
	int admin_id;

public:
	LaptopInfo();
	void operator = (const LaptopInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int adminid);
	void CopyRequest(CustomerRequest request);
	void SetEngineerId(int id);
	void SetAdminId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetLaptopType();
	int GetEngineerId();
	int GetAdminId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class CustomerRecord {
public:
	CustomerRecord();
	void SetRecord(int id, int ordnum);
	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();

private:
	int customer_id;
	int last_order;
};

class Identifier {
public:
	Identifier();
	int Size();

	int GetIdentifier();
	void SetIdentifier(int identifier);

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

private:
	int identifier;
};

class ReplicationRequest {
public:
	ReplicationRequest();
	ReplicationRequest(std::shared_ptr<ServerMetadata> metadata, MapOp op);
	int Size();

	int GetLastIdx();
	int GetCommitedIdx();
	int GetPrimaryId();
	int GetOpCode();
	int GetArg1();
	int GetArg2();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
private:
    int last_idx;
    int committed_idx;
    int primary_id;
    MapOp op;
};

#endif // #ifndef __MESSAGES_H__
