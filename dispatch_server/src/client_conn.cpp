#include "dispatch_server.h"
#include "client_conn.h"
#include "common.h"
#include "sess_cache.h"
#include "base/ef_base64.h"
#include "base/ef_aes.h"
#include "base/ef_md5.h"
#include "base/ef_hex.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "net/ef_event_loop.h"
#include "proto/msg_head.h"
#include "proto/err_no.h"
#include "proto/connect_server.pb.h"

namespace gim{

using namespace ef;
using namespace std;


int CDispatcher::dispatchConnection(EventLoop*l, Connection* c){
	int id = c->getId();
	int idx = 0;	

	idx = m_conf->StartThreadIdx + id % m_conf->ThreadCnt;	
		
	EventLoop& lp = m_s->getEventLoop(idx);
	id = getConnectionId(idx, id);
	c->setId(id);
	lp.asynAddConnection(c);
	return 0;		
}

Connection* CliConFactory::createConnection(EventLoop* l,
		int fd, const string& addr, int port){
	Connection* c = new CliCon(m_serv, m_conf);
	return c;
}

CliCon::~CliCon(){
	atomicDecrement32(&cnt);
}


volatile int CliCon::cnt = 0;


int CliCon::onCreate(EventLoop* l){
	addNotify(l, READ_EVENT);	
	return 0;
}

int CliCon::handleLoginRequest(const head& h, const string& req, 
	string& resp){
	TimeRecorder t("CliCon::handleLoginRequest");
	int ret = 0;
	int i = 0;
	LoginRequest lgreq;
	RedirectResponse rdresp;	
	ConsvCfg csvcfg;

	if(!lgreq.ParseFromArray(req.data()+sizeof(h), 
		req.size()-sizeof(h))){
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}

	if (m_serv->getConsv(lgreq.type(), lgreq.id(), csvcfg) < 0) {
		ret = -1;
		logError("DispatchServer") << "get connect server list fail";
		goto exit;
	}
	
exit:
	rdresp.set_status(ret);
	vector<string>::iterator it = csvcfg.iplist.begin();
	for (; it != csvcfg.iplist.end(); it++) {
		Address *a = rdresp.add_addrs();
		a->set_ip(*it);
		a->set_port(csvcfg.port);
	}
	rdresp.SerializeToString(&resp);
	logError("DispatchServer") << "req:" << req;
	return 0;
}

int CliCon::handlePacket(const string& req){
	int ret = 0;
	head h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);

	string resp;

	try{
		switch(h.cmd){
		case LOGIN_REQ:
			h.cmd = REDIRECT_RESP;
			ret = handleLoginRequest(h, req, resp);		
			break;
		default:
			logError("DisPatchServer") << "<id:" << m_conf->ID
				<< "> <status:unknow_cmd>";
			ret = -1;
		}
		if(resp.size() || h.cmd == KEEPALIVE_REQ){	
			std::string respbuf;
			constructPacket(h, resp, respbuf);
			sendMessage(respbuf);
		}
	}catch(...){
		logError("DispatchServer") << "<action:client_cmd> <conid:" 
			<< getId() << "> <status:exception>";
		ret = -1;
	}
	return ret;
}


int CliCon::checkPackLen(){
	int ret = 0;
	ret = checkLen(*this);
	if(ret <= 0 && bufLen() > 0){
		logError("DispatchServer") << "<action:client_cmd> <event_loop:" 
			<< getEventLoop() << "> <conid:" 
			<< getId() << "> <status:exception>";
		ret = -1;

	}
	return ret;
}

}
