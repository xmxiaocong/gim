#ifndef __SERVER_CONN_H__
#define __SERVER_CONN_H__


#include "base_conn.h"

namespace gim{

using namespace ef;

class SrvCon:public BaseCon 
{
public:
	enum{
		STATUS_INIT = 0,
		STATUS_REG = 1,
	};

	SrvCon(Server* s)
		:BaseCon(s), m_status(STATUS_INIT), 
		m_type(0), m_svid(0){ 
	}

	Server* getServer(){
		return m_serv;
	}

	virtual	~SrvCon();

	virtual int32 onCreate(EventLoop* l);

	virtual int32 handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int32 checkPackLen();

	virtual int32 sendCmd(int32 cmd, const std::string& body);

protected:

private:
	int32 handleRegRequest(const head& h, 
		const std::string& req, std::string& resp);
	int32 handleServiceRequest(const head& h, 
		const std::string& req, std::string& resp);
	int32 handleServiceResponse(const head& h, 
		const std::string& req);
	int32 constructServiceRequestSN(const std::string& oldsn,
		std::string& sn);
	int32 getConFromSessid(const std::string& sessid,
		EventLoop*& l, int32& conid);

	int32 m_status;
	int32 m_type;
	int32 m_svid;
	std::string m_sessid;

};
class   SrvConFactory:public ConnectionFactory{
public:
	SrvConFactory(Server* s, int32 connectionAlivems):m_serv(s){

	}

	~SrvConFactory(){

	}

	virtual Connection* createConnection(EventLoop* l,
                        int32 fd, const std::string& addr, int32 port){
		Connection* c = new SrvCon(m_serv);
		return c;
	}

private:
	Server* m_serv;
};

class	SDispatcher:public ConnectionDispatcher{
public:
	SDispatcher(Server* s):m_s(s){}
	int32 dispatchConnection(EventLoop*l, Connection* c);
private:
	Server* m_s;
};


}

#endif/*SERVER_H*/
