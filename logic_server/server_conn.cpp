#include "server_conn.h"
#include "dispatcher.h"
#include "logic_server.h"
#include "msg_head.h"
#include "err_no.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "connect_server.pb.h"

namespace gim{

using namespace ef;
using namespace std;

int SvCon::onConnected(){

	return doRegister();	
}

SvCon::~SvCon(){
	Dispatcher* d = getDispatcher();
	d->delConnectServer(m_con_serv_id);
}

int SvCon::onCreate(ef::EventLoop* l){
	Client::onCreate(l);
	Dispatcher* d = getDispatcher();
	d->addConnectServer(m_con_serv_id, this);
	return 0;
}

int SvCon::onDisconnected(){
	Dispatcher* d = getDispatcher();
	d->delConnectServer(m_con_serv_id);
	ALogError(m_serv->getConfig().LogName) << "<action:server_disconnect> "
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
	string req;	
	constructPacket(h, "", req);
	ALogError(m_serv->getConfig().LogName) << "<action:keep_alive> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">";
	ret = sendMessage(req);
	return ret;
}

Dispatcher* SvCon::getDispatcher(){
	return (Dispatcher*)getEventLoop()->getObj();
}

int SvCon::doRegister(){
	int ret = 0;
	LoginRequest lgr;
	lgr.set_type(m_sv_type);
	lgr.set_id(ef::itostr(m_svid));
	string reqbody;
	lgr.SerializeToString(&reqbody);	
	head h;
	h.cmd = LOGIN_REQ;
	h.magic = MAGIC_NUMBER;
	string req;
	constructPacket(h, reqbody, req);	
	ALogError(m_serv->getConfig().LogName) << "<action:server_register> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">";
	ret = sendMessage(req);
	return ret;
}

int SvCon::processRegisterResponse(const char* respbody, int len){
	int ret = 0;
	ef::TimeRecorder t("SvCon::handleRegisterResponse");
	LoginResponse lgresp;	
	lgresp.ParseFromArray(respbody, 
		len);
	if(lgresp.status()){
		//reg fail
		disconnect();

		ALogError(m_serv->getConfig().LogName) << "<action:server_register_resp> " 
			"<event_loop:" << getEventLoop() << "> <conid:"
			<< getId() << "> <con_serv_id:" << m_con_serv_id
			<< "> <status:" << lgresp.status() << ">";
		return ret;
	}
	m_status = STATUS_LOGIN; 
	m_sessid = lgresp.sessid();		
	ALogError(m_serv->getConfig().LogName) << "<action:server_register_resp> " 
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< "> <status:0> <sessid:" << m_sessid << ">";  

	return ret;
}

int SvCon::handlePacket(const string& req){
	int ret = 0;
	head h;
	h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	h.magic = htonl(h.magic);

	try{
		switch(h.cmd){
		case LOGIN_RESP:
			ret = processRegisterResponse(req.data() + sizeof(h),
				req.size() - sizeof(h));		
			break;
		case SERVICE_REQ:
			ret = processServiceRequest(req.data() + sizeof(h), 
				req.size() - sizeof(h));
			break;
		case SERVICE_RESP:
			ret = processServiceResponse(req.data() + sizeof(h),
				req.size() - sizeof(h));
			break;
		case KEEPALIVE_RESP:
			break;	
		default:
			ret = -1;
			break;
		}
	}catch(exception const& e){
		ALogError(m_serv->getConfig().LogName) << "<action: handle_packet> Exception:"
			<< e.what();	
	}catch(...){
		ALogError(m_serv->getConfig().LogName) << "<action: handle_packet> Exception!";	
	}
	return ret;
}

int SvCon::processServiceRequest(const char* reqbody, int len){
	int ret = 0;
	ServiceRequest svreq;

	if(!svreq.ParseFromArray(reqbody, len)){
		ALogError(m_serv->getConfig().LogName) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:INPUT_FORMAT_ERROR>";
		return INPUT_FORMAT_ERROR;	
	}
	
	if(svreq.to_type() != m_sv_type){
		
		ALogError(m_serv->getConfig().LogName) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:SERVICE_TYPE_ERROR>";
		return SERVICE_TYPE_ERROR;
	}

	ret = handleServiceRequest(svreq);

	return ret; 
	
}

int SvCon::processServiceResponse(const char* respbody, int len){
	int ret = 0;
	ServiceResponse svreq;

	if(!svreq.ParseFromArray(respbody, len)){
		ALogError(m_serv->getConfig().LogName) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:INPUT_FORMAT_ERROR>";
		return INPUT_FORMAT_ERROR;	
	}

	ret = handleServiceResponse(svreq);

	return ret; 

}

int SvCon::sendServiceResponse(const ServiceResponse& resp){
	int ret = 0;
	string body;

	if(m_status != STATUS_LOGIN){
		return -1;
	}

	ServiceResponse resp1 = resp;
	resp1.set_from_sessid(m_sessid);
	resp1.set_from_type(m_sv_type);
	resp1.SerializeToString(&body);
	
	head resph;
	resph.cmd = SERVICE_RESP;
	resph.magic = MAGIC_NUMBER;

	string msg;
	constructPacket(resph, body, msg);
	
	ret = sendMessage(msg);

	return ret;

}


int SvCon::sendServiceRequest(const ServiceRequest& req){
	int ret = 0;
	string body;

	ServiceRequest req1 = req;

	req1.set_from_sessid(m_sessid);
	req1.set_from_type(m_sv_type);

	req1.SerializeToString(&body);
	
	head resph;
	resph.cmd = SERVICE_REQ;
	resph.magic = MAGIC_NUMBER;	

	string msg;
	constructPacket(resph, body, msg);

	ret = sendMessage(msg);

	return ret;

}

int SvCon::callService(const ServiceRequest& req, ServiceResponse& resp, 
	int timeout_ms){
	
	Dispatcher* d = getDispatcher();	

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->callService(req, resp, timeout_ms);

	return ret;
	
}

int SvCon::sendToClient(const std::string& cid, const ServiceRequest& req){

	Dispatcher* d = getDispatcher();	

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendToClient(cid, req);

	return ret;
	
}

int SvCon::sendToClient(const std::string& cid, const ServiceResponse& resp){

	Dispatcher* d = getDispatcher(); 

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendToClient(cid, resp);

	return ret;
}

int SvCon::checkPackLen(){
	int ret = 0;

	head h;

	if(bufLen() < (int)sizeof(head)){
		return 0;
	}

	peekBuf((char*)&h, sizeof(head));
	h.magic = htonl(h.magic);
	h.len = htonl(h.len);

	if(h.len < (int)sizeof(h) 
		|| h.len > 1024 * 1024){
		ret = -1;
	} else if(h.len <= bufLen()){
		ret = h.len;
	}

	if(ret <= 0 && bufLen() > 0){
		ALogError(m_serv->getConfig().LogName) << "<action:client_check_pack> " 
			"<event_loop:" << getEventLoop() << "> <conid:" 
			<< getId() << "> <buflen:" << bufLen() 
			<< "> <status:packet_not_full>";
	}

	return ret;
}

}
