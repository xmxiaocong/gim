#ifndef __LOGIC_CONN_H__
#define __LOGIC_CONN_H__

#include "net/ef_connection.h"
#include "net/ef_acceptor.h"

namespace gim{

using namespace ef;

class LogicServer;
class Dispatcher;

class LogicCon: public Connection{
public:
	LogicCon():m_serv(NULL){};

	virtual ~LogicCon(){};

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}

	LogicServer* getLogicServer(){
		return m_serv;
	}

	int sendServiceRequestToClient(const std::string& cid,
		int svtype, const std::string& sn, 
		const std::string& payload,
		const std::string& callback);
	
	int sendServiceResponseToClient(const std::string& cid,
		int svtype, int status, const std::string& sn, 
		const std::string& payload,
		const std::string& callback);

private:

	Dispatcher* getDispatcher();

	LogicServer* m_serv;
	
};

class LogicConFactory: public ConnectionFactory{
public:
	LogicConFactory():m_serv(NULL){};
	virtual ~LogicConFactory(){};

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}

	LogicServer* getLogicServer(){
		return m_serv;
	}

	virtual Connection* createConnection(EventLoop* l,
		int fd, const std::string& addr, int port) = 0;
	
private:
	LogicServer* m_serv;
};

};


#endif
