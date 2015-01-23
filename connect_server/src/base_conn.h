#ifndef __BASE_CONN_H__
#define __BASE_CONN_H__

#include "net/ef_server.h"
#include "net/ef_operator.h"
#include "proto/msg_head.h"
#include "common.h"


namespace gim{

class BaseCon:public Connection{
public:
	BaseCon(Server* s):m_serv(s){};

	virtual ~BaseCon(){};

	virtual int32 sendCmd(int32 cmd, const std::string& body) = 0;
protected:
	Server* m_serv;
};



class SendMsgOp: public NetOperator{
public:
	SendMsgOp(int32 conid, int32 cmd, const std::string& body)
		: m_conid(conid), m_cmd(cmd), m_body(body){
	}

	virtual int32 process(EventLoop *l){
		BaseCon* clic = (BaseCon*)l->getConnection(m_conid);
		if(!clic){
			return -1;
		}
		return clic->sendCmd(m_cmd, m_body);
	}

private:
	int32 m_conid;
	int32 m_cmd;
	std::string m_body;
};


}

#endif
