#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__

#include "base/ef_atomic.h"
#include "base/ef_btype.h"
#include "net/ef_connection.h"
#include "msg_head.h"
#include "net/ef_server.h"
#include "push_def.h"
#include <string>
#include <json/json.h>
#include "message.pb.h"

namespace ef{
	class EventLoop;
};

namespace gim{

class head;
class Dispatcher;

class PushCliConn
	:public ef::Connection
{
public:
	PushCliConn()
		:m_busy(0){
		ef::atomicIncrement32(&cnt);
	}
	virtual	~PushCliConn();

	virtual int onCreate(ef::EventLoop* l);

	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	static int totalCount(){
		return cnt;
	}	
private:
	int handleRequest(const std::string& jstr);
	int handlePush(const Json::Value& vrequest);
	int handleKeepalive(const head& h);
	Dispatcher*  getDispatcher();
	int sendJ2C(const Message& m);
private:
	int m_busy;
	ef::int64 m_req_cnt;
	ef::int64 m_resp_cnt;
	
	static volatile int cnt;
};

class PushCliConFactory
	:public ef::ConnectionFactory
{
public:
	PushCliConFactory(){};

	~PushCliConFactory(){};

	virtual ef::Connection* createConnection(ef::EventLoop* l,
			int fd, const std::string& addr, int port);
};


class PushCliConnDispatcher
	:public ef::ConnectionDispatcher
{
public:
	PushCliConnDispatcher(ef::Server* s)
		:m_s(s){};
	int dispatchConnection(ef::EventLoop*l, ef::Connection* c);
private:
	ef::Server* m_s;
};

};//end of namespace ef

#endif
