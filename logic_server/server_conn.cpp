#include "server_conn.h"
#include "dispatcher.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "logic_server.h"
#include "msg_head.h"
#include "err_no.h"
#include "connect_server.pb.h"

namespace gim{

using namespace ef;


int SvCon::onConnected(){
	return doRegister();	
}

int SvCon::onDisconnected(){
	Dispatcher* d = m_serv->getDispatcher();
	d->delConnectServer(m_con_serv_id);
	ALogError(m_serv->getLogName()) << "<action:server_disconnect> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">"; 	
	return 0;
}

int SvCon::keepAlive(){
	int ret = 0;
	head h;
	h.magic = MAGIC_NUMBER;
	h.cmd = KEEPALIVE_REQ;	
	std::string req;	
	constructReqPacket(h, "", req);
	ALogError(m_serv->getLogName()) << "<action:keep_alive> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">";
	ret = sendMessage(req);
	return ret;
}

int SvCon::doRegister(){
	int ret = 0;
	SvRegRequest lgr;
	lgr.set_svtype(m_service_type);
	std::string reqbody;
	lgr.SerializeToString(&reqbody);	
	head h;
	h.cmd = SERVICE_REG_REQ;
	h.magic = MAGIC_NUMBER;
	std::string req;
	constructReqPacket(h, reqbody, req);	
	ALogError(m_serv->getLogName()) << "<action:server_register> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">";
	ret = sendMessage(req);
	return ret;
}

int SvCon::handleRegisterResponse(const char* respbody, int len){
	int ret = 0;
	ef::TimeRecorder t("SvCon::handleRegisterResponse");
	SvRegResponse lgresp;	
	lgresp.ParseFromArray(respbody, 
		len);
	if(lgresp.status()){
		//reg fail
		disconnect();

		ALogError(m_serv->getLogName()) << "<action:server_register_resp> " 
			"<event_loop:" << getEventLoop() << "> <conid:"
			<< getId() << "> <con_serv_id:" << m_con_serv_id
			<< "> <status:" << lgresp.status() << ">";
	}else{
		Dispatcher* d = m_serv->getDispatcher();
		d->addConnectServer(m_con_serv_id, getEventLoop(), getId());	
		m_status = STATUS_LOGIN; 
		
		ALogError(m_serv->getLogName()) << "<action:server_register_resp> " 
			"<event_loop:" << getEventLoop() << "> <conid:"
			<< getId() << "> <con_serv_id:" << m_con_serv_id
			<< "> <status:0>";  
	}
	return ret;
}

int SvCon::handlePacket(const std::string& req){
	int ret = 0;
	head h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	h.magic = htonl(h.magic);
	std::string resp;
	try{
		switch(h.cmd){
		case SERVICE_REG_RESP:
			ret = handleRegisterResponse(req.data() + sizeof(h),
				req.size() - sizeof(h));		
			break;
		case SERVICE_REQ:
			ret = handleServiceRequest(req.data() + sizeof(h), 
				req.size() - sizeof(h), resp);
			break;
		case SERVICE_RESP:
			ret = handleServiceResponse(req.data() + sizeof(h),
				req.size() - sizeof(h));
			break;
		case KEEPALIVE_RESP:
			break;	
		default:
			ret = -1;
		}
		if(resp.size()){	
			std::string respbuf;
			head resph = h;
			resph.cmd = h.cmd + 1;
			constructPacket(resph, resp, respbuf);
			ret = sendMessage(respbuf);
		}
	}catch(...){
		ret = -1;
	}
	return ret;
}

int checkLen(ef::Connection& c){
	head h;
	if(c.bufLen() < (int)sizeof(h)){
		return 0;
	}
	c.peekBuf((char*)&h, sizeof(h));
	h.magic = htonl(h.magic);
	h.len = htonl(h.len);
	if(h.len < (int)sizeof(h) 
		|| h.len > 1024 * 1024){
		return -1;
	}
	if(h.len <= c.bufLen()){
		return h.len;
	}
	return 0;
}

int SvCon::checkPackLen(){
	int ret = 0;
	ret = checkLen(*this);
	if(ret <= 0 && bufLen() > 0){
		ALogError(m_serv->getLogName()) << "<action:client_check_pack> " 
			"<event_loop:" << getEventLoop() << "> <conid:" 
			<< getId() << "> <buflen:" << bufLen() 
			<< "> <status:packet_not_full>";
	}

	return ret;
}

}
