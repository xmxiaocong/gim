#ifndef PEER_CONN_H
#define PEER_CONN_H

#include "net/ef_connection.h"
#include "net/ef_acceptor.h"

namespace gim{

using namespace ef;

struct head;
class LogicServer;
class Dispatcher;

class PushCon:public Connection{
public:
	PushCon():m_serv(NULL){
	}

	virtual int32 onCreate(ef::EventLoop* l);
	virtual int32 handleTimer(ef::EventLoop* l, int32 id){
		return 0;
	}
	virtual int32 handlePacket(const std::string& req);
	virtual int32 checkPackLen();

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}

	LogicServer* getLogicServ(){
		return m_serv;
	}

	Dispatcher* getDispatcher();

private:
	int32 handlePushMessage(const head& h, const std::string& svreq, 
		std::string& resp);

	LogicServer* m_serv;
};

class PushConFac:public ConnectionFactory{
public:
	virtual PushCon* createConnection(ef::EventLoop*, ef::int32, 
		const std::string&, ef::int32){

		PushCon* pc = new PushCon();
		pc->setLogicServer(m_serv);
	
		return pc;
	}

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}
private:
	LogicServer* m_serv;
};

PushConFac* getPushConFactory();

};

#endif/*PEER_CONN_H*/
