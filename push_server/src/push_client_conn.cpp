#include "push_client_conn.h"
#include "net/ef_event_loop.h"
#include "connect_server.pb.h"
#include "logic_common.h"
#include "dispatcher.h"
#include "base/ef_log.h"
#include "push_def.h"
#include "push_common.h"
#include "push_msg_db.h"
#include "msg_db.h"

namespace gim{
int PushCliConnDispatcher::dispatchConnection(EventLoop*l, Connection* c){
	int id = c->getId();

	int idx = id % m_s->getEventLoopCount();
	
		
	EventLoop& lp = m_s->getEventLoop(idx);
	lp.asynAddConnection(c);
	return 0;		
}

Connection* PushCliConFactory::createConnection(EventLoop* l,int fd, const std::string& addr, int port){
	Connection* c = new PushCliConn();
	return c;
}

PushCliConn::~PushCliConn(){
	atomicDecrement32(&cnt);
}


volatile int PushCliConn::cnt = 0;


int PushCliConn::onCreate(EventLoop* l){
	addNotify(l, READ_EVENT);	
	return 0;
}



int PushCliConn::handlePacket(const std::string& req){

	int ret = 0;
	head h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	h.magic = htonl(h.magic);
	
	if(h.magic != MAGIC_NUMBER){
		PSLogError("PushServer") << "<action:handlePacket> <conid:" 
			<< getId() << "> <status:wrong migic>";
		return -1;
	}

	std::string resp;

	try{
		switch(h.cmd){
		case SERVICE_REQ:
			ret = handleRequest(req.substr(sizeof(h)));
			break;
		case KEEPALIVE_REQ:
			handleKeepalive(h);
			break;
		default:
			PSLogError("PushServer") << "<action:handlePacket> <conid:" 
				<< getId() << "> <status: undefined head cmd>";
			ret = -1;
		}
	}catch(...){
		PSLogError("PushServer") << "<action:handlePacket> <conid:" 
			<< getId() << "> <status:exception>";
		ret = -1;
	}
	return ret;
}

int PushCliConn::handleRequest(const std::string& jstr){
	int ret = -1;
	try{
		Json::Value v;
		Json::Reader r;
		if(!r.parse(jstr, v)){
			PSLogError("PushServer") << "<action:handleRequest> <conid:" 
				<< getId() << "> <status: parse json error!>";
			ret = -1;
			goto req_err;
		}
		
		int cmd = v["cmd"].asInt();
		switch(cmd){
				case PS_CMD_CALL_PUSH:
					ret = handlePush(v);
					break;
				default:
					PSLogError("PushServer") << "<action:handleRequest> <conid:" 
						<< getId() << "> <status: undefined json cmd>";
					ret = PS_RESULT_UNDEFINED_CMD;
					break;
		}
	}catch(const std::exception& e){
		PSLogError("PushServer") << "<action:handleRequest> <conid:" 
			<< getId() << "> <status:exception: " << e.what() << ">";
		ret = PS_RESULT_JSON_ERROR;
		goto req_err;
	}

req_err:
	if(ret < 0){
		Json::Value response;
		response["cmd"] = PS_CMD_CALL_PUSH_RESP;
		response["result"] = ret;
		std::string respbuf;
		head rh;
		rh.magic = MAGIC_NUMBER;
		rh.cmd = SERVICE_RESP;
		rh.len = 0;
		constructPacket(rh, Json::FastWriter().write(response), respbuf);
		sendMessage(respbuf);
	}
	return 0;
}

int PushCliConn::handlePush(const Json::Value& vrequest){
	//PSLogTrace("PushServer") << "<action:handlePush> <conid:" << getId() << ">";

	Dispatcher* d = getDispatcher();
	if(!d){
		PSLogError("PushServer") << "<action:handlePush> <conid:" 
			<< getId() << "> <status: Dispatcher:NULL>";
		return -1;
	}

	if(!pushRequestJsonCheck(vrequest)){
		PSLogError("PushServer") << "<action:handlePush> <conid:" 
			<< getId() << "> <status:  pushRequestJsonCheck error>";
		return PS_RESULT_ARGS_ERROR;
	}
	
	Message msg;
	messageJsonToProtobuf(vrequest,msg);

	const std::string& to = msg.to();
	const std::string& sn = msg.sn();

	std::string mkey = MSG_KEY(to);
	std::string idkey = LAST_MSG_ID_KEY(to);

	int64 msgid =-1;
	int ret = PushMsgDBM::MQ()->incrId(to, idkey, msgid);
	if(ret < 0){
		PSLogError("PushServer") << "<action:handlePush> <conid:" 
			<< getId() << "> <status: incrId error:" << ret << ">";
		return PS_RESULT_INCR_MSGID_ERROR;
	}
	
	msg.set_id(msgid);
	ret  = sendJ2C(msg);

	if(msg.expire() > 0){
		if(PushMsgDBM::MQ()->add(to, mkey, msg) < 0){
			PSLogError("PushServer") << "<action:handlePush> <conid:" 
				<< getId() << "> <status: addMsg error:>";
		}
	}

	Json::Value response;
	response["cmd"] = PS_CMD_CALL_PUSH_RESP;
	response["result"] = ret;

	response["sn"] = sn;
	response["msgid"] = ef::itostr(msgid);
	std::string respbuf;
	head rh;
	rh.magic = MAGIC_NUMBER;
	rh.cmd = SERVICE_RESP;
	rh.len = 0;
	constructPacket(rh, Json::FastWriter().write(response), respbuf);
	ret = sendMessage(respbuf);

	return 0;
}
int PushCliConn::sendJ2C(const Message& m){
	Json::Value v;
	v["cmd"] = PS_CMD_PUSH_REQUEST;
	messageProtobufToJson(m, v);

	ServiceRequest sreq;
	sreq.set_payload(Json::FastWriter().write(v));
	sreq.set_to_type(-1);
	sreq.set_sn(m.sn());

	int ret = getDispatcher()->sendToClient(m.to(), sreq);
	PSLogTrace("PushServer") << "<action:handle push send2c> <conid:" << getId() << "><to:" << m.to() << "><result=" << ret << ">";
	return ret == -20001?0:ret;
}

int PushCliConn::handleKeepalive(const head& h){
	std::string respbuf;
	constructPacket(h, "", respbuf);
	return sendMessage(respbuf);
}

Dispatcher*  PushCliConn::getDispatcher(){
	return (Dispatcher*)Connection::getEventLoop()->getObj();
}

int PushCliConn::checkPackLen(){
	head h;
	if(bufLen() < (int)sizeof(h)){
		return 0;
	}
	peekBuf((char*)&h, sizeof(h));
	h.magic = htonl(h.magic);
	h.len = htonl(h.len);
	if(h.len < (int)sizeof(h) 
		|| h.len > 1024 * 1024){
			return -1;
	}
	if(h.len <= bufLen()){
		return h.len;
	}
	return 0;
}

}
