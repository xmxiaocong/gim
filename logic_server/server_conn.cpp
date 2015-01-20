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


int SvCon::onConnected(){
	return doRegister();	
}

int SvCon::onDisconnected(){
	Dispatcher* d = getDispatcher();
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
	constructPacket(h, "", req);
	ALogError(m_serv->getLogName()) << "<action:keep_alive> "
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
	SvRegRequest lgr;
	lgr.set_svtype(m_service_type);
	lgr.set_id(m_svid);
	std::string reqbody;
	lgr.SerializeToString(&reqbody);	
	head h;
	h.cmd = SERVICE_REG_REQ;
	h.magic = MAGIC_NUMBER;
	std::string req;
	constructPacket(h, reqbody, req);	
	ALogError(m_serv->getLogName()) << "<action:server_register> "
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< ">";
	ret = sendMessage(req);
	return ret;
}

int SvCon::processRegisterResponse(const char* respbody, int len){
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
		return ret;
	}
	Dispatcher* d = getDispatcher();
	d->addConnectServer(m_con_serv_id, getId());	
	m_status = STATUS_LOGIN; 
	m_sessid = lgresp.sessid();		
	ALogError(m_serv->getLogName()) << "<action:server_register_resp> " 
		"<event_loop:" << getEventLoop() << "> <conid:"
		<< getId() << "> <con_serv_id:" << m_con_serv_id
		<< "> <status:0> <sessid:" << m_sessid << ">";  

	return ret;
}

int SvCon::handlePacket(const std::string& req){
	int ret = 0;
	head h;
	h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	h.magic = htonl(h.magic);

	m_from_sessid.clear();
	m_cur_pack_sn.clear();
	m_cur_pack_key.clear();
	m_callback.clear();
	m_packsvtype = 0;

	try{
		switch(h.cmd){
		case SERVICE_REG_RESP:
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
		}
	}catch(...){
		ret = -1;
	}
	return ret;
}

int SvCon::processServiceRequest(const char* reqbody, int len){
	int ret = 0;
	ServiceRequest svreq;

	if(!svreq.ParseFromArray(reqbody, len)){
		ALogError(m_serv->getLogName()) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:INPUT_FORMAT_ERROR>";
		return INPUT_FORMAT_ERROR;	
	}

	m_from_sessid = svreq.from_sessid();
	m_cur_pack_sn = svreq.sn();
	m_cur_pack_key = svreq.key();
	m_packsvtype = svreq.svtype();
	m_callback = svreq.callback();	
	
	
	if(svreq.svtype() != m_service_type){
		
		ALogError(m_serv->getLogName()) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:SERVICE_TYPE_ERROR>";
		return SERVICE_TYPE_ERROR;
	}

	ret = handleServiceRequest(svreq.payload());

	return ret; 
	
}

int SvCon::processServiceResponse(const char* respbody, int len){
	int ret = 0;
	ServiceResponse svreq;

	if(!svreq.ParseFromArray(respbody, len)){
		ALogError(m_serv->getLogName()) << "<action:handle_cmd> <status:"
			<< (int)INPUT_FORMAT_ERROR
			<< "> <errstr:INPUT_FORMAT_ERROR>";
		return INPUT_FORMAT_ERROR;	
	}

	m_cur_pack_sn = svreq.sn();
	m_packsvtype = svreq.svtype();
	m_callback = svreq.callback();	

	ret = handleServiceResponse(svreq.svtype(), svreq.status(), 
		svreq.payload(), m_callback);

	return ret; 

}

int SvCon::sendServiceResponse(int status, const std::string& payload){
	int ret = 0;
	ServiceResponse svresp;

	svresp.set_sn(getPackSN());
	svresp.set_svtype(m_service_type);
	svresp.set_from_sessid(m_sessid);	
	svresp.set_to_sessid(m_from_sessid);
	svresp.set_payload(payload);
	svresp.set_callback(getPackCallBack());
	svresp.set_status(status);

	std::string body;
	
	svresp.SerializeToString(&body);
	
	head resph;
	resph.cmd = SERVICE_RESP;
	resph.magic = MAGIC_NUMBER;

	std::string msg;
	constructPacket(resph, body, msg);
	
	ret = sendMessage(msg);

	return ret;
}


int SvCon::sendServiceRequest(int svtype, const std::string& key,
	const std::string& payload, const std::string& callback){
	int ret = 0;
	
	ServiceRequest svresp; 
	svresp.set_sn(getPackSN());
	svresp.set_svtype(svtype);
	svresp.set_from_sessid(m_sessid);
	svresp.set_key(key);
	svresp.set_payload(payload);
	svresp.set_callback(callback);


	std::string body;
	svresp.SerializeToString(&body);

	head resph;
	resph.cmd = SERVICE_REQ;
	resph.magic = MAGIC_NUMBER;	

	std::string msg;
	constructPacket(resph, body, msg);

	ret = sendMessage(msg);

	return ret;

}

int SvCon::sendServiceRequestToClient(const string& cid, const string& sn,
	const string& payload, const std::string& callback){

	Dispatcher* d = getDispatcher();	

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendServiceRequestToClient(cid, m_service_type,
		sn, payload, callback);

	return ret;
	
}

int SvCon::sendServiceResponseToClient(const string& cid, const std::string& sn, int status, 
	const string& payload, const std::string& callback){

	Dispatcher* d = getDispatcher(); 

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendServiceResponseToClient(cid, m_service_type,
		sn, status, payload, callback);

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
		ALogError(m_serv->getLogName()) << "<action:client_check_pack> " 
			"<event_loop:" << getEventLoop() << "> <conid:" 
			<< getId() << "> <buflen:" << bufLen() 
			<< "> <status:packet_not_full>";
	}

	return ret;
}

}
