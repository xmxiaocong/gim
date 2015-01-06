#include "server_conn.h"
#include "server_manager.h"
#include "client_conn.h"
#include "cache_conn.h"
#include "settings.h"
#include "common.h"
#include "base/ef_statistic.h"
#include "proto/connect_server.pb.h"
#include "proto/err_no.h"

namespace gim{

int32 SDispatcher::dispatchConnection(EventLoop*l, Connection* c){
	int32 id = l->getConId();
	int32 idx = 0;
	int32 evlpcnt = m_s->getEventLoopCount();

	if(evlpcnt > 1){
		int32 evcnt = serverEventLoopCount(evlpcnt);
		idx = id % evcnt;
	}

	EventLoop& lp = m_s->getEventLoop(idx);
	id = getConnectionId(idx, id);
	c->setId(id);
	lp.asynAddConnection(id, c);
	return 0;		
}

int32 SrvCon::onCreate(EventLoop* l){
	return addNotify(l, READ_EVENT);
}

SrvCon::~SrvCon(){
	if(m_status == STATUS_REG){
		getServMan().delServer(m_type, this, 
			getEventLoop());
		m_status = STATUS_INIT;
		ALogError("ConnectServer") << "<action:server_logout> <svtype:" 
			<< m_type << "> <event_loop:"<< getEventLoop() 
			<< "> <conid:" << getId() << "> <status:success>"; 
	}
}

int32 SrvCon::checkPackLen(){
	return checkLen(*this);
}

int32 SrvCon::handlePacket(const std::string& req){
	int32 ret = 0;
	head h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	std::string resp;
	try{
		switch(h.cmd){
		case SERVICE_REG_REQ:
			ret = handleRegRequest(h, req, resp);
			break;
		case SERVICE_REQ:
			ret = handleServiceRequest(h, req, resp);
			break;
		case SERVICE_RESP:
			ret = handleServiceResponse(h, req); 
			break;
		case KEEPALIVE_REQ: 
			break;
		default:
			ALogError("ConnectServer") 
				<< "<action:server_cmd> <svtype:" 
				<< m_type << "> <status:unknow_cmd>";
			ret = -1;
		}
		if(resp.size() || h.cmd == KEEPALIVE_REQ){
			std::string respbuf;
			head resph = h;
			resph.cmd = h.cmd + 1;
			constructPacket(resph, resp, respbuf);
			sendMessage(respbuf);
		}
	}catch(...){
		ALogError("ConnectServer") << "<action:server_cmd> <svtype:" 
			<< m_type << "> <event_loop:"<< getEventLoop() 
			<< "> <conid:" << getId() << "> <status:exception>"; 
		ret = -1;
	}
	return ret;	
}

int32 SrvCon::getCliConFromSessid(const std::string& sessid, 
	EventLoop*& l, int32& conid){
	int32 ret = 0;
	int32 svid = 0;
	int32 evlpid = 0;
	std::string cid;

	Settings* setting = Singleton<Settings>::instance();
	ret = getDecorationInfo(sessid, svid, conid, cid);	

	if(ret < 0 || svid != setting->Id){
		ret = INVLID_SESSION_ID;
		goto exit;	
	}

	evlpid = getEventLoopId(conid);

	if(evlpid < serverEventLoopCount(m_serv->getEventLoopCount())
		|| evlpid >= m_serv->getEventLoopCount()){
		ret = INVLID_SESSION_ID;
		goto exit;
	}


	l = &(m_serv->getEventLoop(evlpid));
exit:
	return ret;	
}

int32 SrvCon::constructServiceRequestSN(const std::string& oldsn, 
	std::string& sn){

	Settings *pSettings = Singleton<Settings>::instance();
	decorationName(pSettings->Id, getId(), oldsn, sn); 

	return 0;	
}

int32 SrvCon::handleRegRequest(const head& h, 
		const std::string& req, std::string& resp){

	TimeRecorder t("SrvCon::handleRegRequest");
	int32 ret = 0;
	SvRegRequest regreq;

	if(!regreq.ParseFromArray(req.data()+sizeof(h), 
		req.size()-sizeof(h))){
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}

	m_type = regreq.svtype();
	ret = getServMan().addServer(m_type, this, getEventLoop());

	if(ret >= 0){
		m_status = STATUS_REG;
	}

exit:
	SvRegResponse regrsp;
	regrsp.set_status(ret);
	regrsp.SerializeToString(&resp);
	ALogError("ConnectServer") << "<action:server_login> <svtype:"
		<< m_type << "> <event_loop:"<< getEventLoop() 
		<< "> <conid:" << getId() << "> <status:" << ret 
		<< "> <errstr:" << getErrStr(ret) << ">";
	return 0;		
}


int32 SrvCon::handleServiceRequest(const head& h, 
		const std::string& req, std::string& resp){
	TimeRecorder t("SrvCon::handleServiceRequest");
	int32 ret = 0;
	ServiceRequest svreq;
	ServiceRequest newsvreq;
	ServiceResponse svresp;
	std::string newsn;
	std::string newreq;
	std::string reqbuf;
	EventLoop* l = NULL;
	CliRespOp* op = NULL;
	int32 conid = 0;

	svresp.set_sessid("null");
	svresp.set_svtype(0);
	svresp.set_sn("null");
	if(!svreq.ParseFromArray(req.data()+sizeof(h), req.size()-sizeof(h))){
		svresp.set_status(INPUT_FORMAT_ERROR);
		ret = INPUT_FORMAT_ERROR;	
		goto exit;
	}

	svresp.set_sessid(svreq.sessid());
	svresp.set_svtype(svreq.svtype());
	svresp.set_sn(svreq.sn());

	ret = getCliConFromSessid(svreq.sessid(), l, conid);
	if(ret < 0){
		svresp.set_status(ret);
		ret = 0;
		goto exit;
	}

	constructServiceRequestSN(svreq.sn(), newsn);
	newsvreq = svreq;
	newsvreq.set_sn(newsn);	
	newsvreq.SerializeToString(&newreq);
	//constructReqPacket(h, newreq, reqbuf);
	op = new CliRespOp(conid, SERVICE_REQ, newreq);	
	ret = l->addAsynOperator(op);
	if(ret < 0){
		svresp.set_status(ret);
		ret = 0;
		goto exit;
	}
exit:
	if(svresp.status()){
		svresp.SerializeToString(&resp);
	}
	ALogError("ConnectServer") 
		<< "<action:server_service_request> <sessid:" 
		<< svresp.sessid() 
		<< "> <svtype:" << svresp.svtype() 
		<< "> <sn:" << svreq.sn() << "> <new_sn:" << newsn
		<< "> <status:" << svresp.status()
		<< "> <errstr:" << getErrStr(svresp.status()) << ">";
	return ret; 
}

int32 SrvCon::handleServiceResponse(const head& h, 
		const std::string& req){
	TimeRecorder t("SrvCon::handleServiceResponse");
	int32 ret = 0;
	ServiceResponse svresp;
	EventLoop* l = NULL;
	CliRespOp* op = NULL;
	int32 conid = 0;
	if(!svresp.ParseFromArray(req.data()+sizeof(h), req.size()-sizeof(h))){
		ret = INPUT_FORMAT_ERROR;	
		goto exit;
	}

	ret = getCliConFromSessid(svresp.sessid(), l, conid);
	if(ret < 0){
		goto exit;
	}
	op = new CliRespOp(conid, SERVICE_RESP, req.substr(sizeof(h)));
	ret = l->addAsynOperator(op);
exit:
	ALogError("ConnectServer")  
		<< "<action:server_service_response> <sessid:" 
		<< svresp.sessid() << "> <svtype:" << svresp.svtype()
		<< "> <sn:" << svresp.sn() << "> <status:" << ret
		<< "> <errstr:" << getErrStr(ret) << ">";
	if(ret != INPUT_FORMAT_ERROR)
		ret = 0;
	return ret;
}

}
