#ifndef PEER_CONN_H
#define PEER_CONN_H

#include "logic_conn.h"

namespace gim{


struct head;
class LogicServer;
class Dispatcher;

class PushCon:public LogicCon{
public:
	PushCon():m_serv(NULL){
	}

	virtual int32 onCreate(ef::EventLoop* l);
	virtual int32 handleTimer(ef::EventLoop* l, int32 id){
		return 0;
	}
	virtual int32 handlePacket(const std::string& req);
	virtual int32 checkPackLen();


private:
	int32 handlePushMessage(const head& h, const std::string& svreq, 
		std::string& resp);

	LogicServer* m_serv;
};

class PushConFac:public LogicConFactory{
public:
	virtual PushCon* createConnection(ef::EventLoop*, ef::int32, 
		const std::string&, ef::int32){

		PushCon* pc = new PushCon();
		pc->setLogicServer(getLogicServer());
	
		return pc;
	}

private:
};

PushConFac* getPushConFactory();

};

#endif/*PEER_CONN_H*/
