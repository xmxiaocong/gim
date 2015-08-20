#include "rpc_client.h"
#include "msg_head.h"
#include "err_no.h"
#include "logic_common.h"
#include "connect_server.pb.h"


namespace gim{

using namespace ef;

RPCClient::RPCClient()
	:m_server_port(0), m_sock(EF_INVALID_SOCKET), m_retrytime(3){
	
}

RPCClient::~RPCClient(){
	disconnect();
}

int RPCClient::init(const std::string& ip, int port, int retrytime){
	m_server_ip = ip;
	m_server_port = port;
	m_retrytime = retrytime;

	int ret = reconnect();

	//std::cout << "++++RPCClient::init: ip :"<< ip << ", port:" << port << ", ret:" << ret << std::endl;
	
	return ret;
}

int RPCClient::reconnect(){

	if(m_sock >= 0){
		return m_sock;
	}

	m_sock = tcpConnectWithTimeout(m_server_ip.data(), m_server_port, 100);

	if(m_sock < 0){
		return m_sock;
	}

	int ret = login();

	if(ret < 0){
		disconnect();
	}

	return ret;
}

int RPCClient::login(){
	int cmd = 0;
	LoginRequest lgreq;
	LoginResponse lgresp;

	lgreq.set_type(0);
	lgreq.set_id("RPC");
	lgreq.set_version("0.0.0");

	std::string req;
	std::string resp;

	bool b = lgreq.SerializeToString(&req);
	if(!b){
		return INPUT_FORMAT_ERROR;
	}

	int ret = doSend(LOGIN_REQ, req, 100);

	if(ret < 0){
		return ret;
	}

	ret = doRecv(cmd, resp, 100);
	if(ret < 0){
		return ret;
	}	

	b = lgresp.ParseFromString(resp);
	if(!b){
		return INPUT_FORMAT_ERROR;
	}

	m_sessid = lgresp.sessid();

	return 0;
	
}

int RPCClient::callService(const ServiceRequest& req, ServiceResponse& resp, 
	int timeout_ms){
	reconnect();
	int ret = 0;
	for(int i = 0; i < m_retrytime; ++i){
		ret = doCallService(req, resp, timeout_ms);
		if(ret >= 0)
			break;
	}

	return ret;
}
	
int RPCClient::doCallService(const ServiceRequest& req, ServiceResponse& resp, 
	int timeout_ms){

	std::string reqmsg;
	std::string respmsg;
	int cmd = 0;

	ServiceRequest req1;
	req1 = req;
	req1.set_from_sessid(m_sessid);
	req1.set_from_type(0);

	bool b = req1.SerializeToString(&reqmsg);
	resp.set_from_sessid("");
	resp.set_status(0);
	resp.set_sn(req.sn());
	resp.set_to_sessid(req1.from_sessid());
	resp.set_to_type(0);
	if(!b){
		resp.set_status(INPUT_FORMAT_ERROR);
		return INPUT_FORMAT_ERROR;
	}

	int ret = doSend(SERVICE_REQ, reqmsg, timeout_ms);

	if(ret < 0){
	//disconnect
		goto dis;
	}

	ret = doRecv(cmd, respmsg, timeout_ms);
	if(ret < 0){
		//disconnect
		goto dis;
	}

	b = resp.ParseFromString(respmsg);
	if(!b){
		resp.set_status(INPUT_FORMAT_ERROR);
		return INPUT_FORMAT_ERROR;
	}

	return ret;
dis:
	//std::cout << "RPCClient::doCallService disconnect!\n";
	resp.set_status(INNER_ERROR);
	disconnect();
	return INNER_ERROR;
	
}


int RPCClient::disconnect(){
	if(m_sock != EF_INVALID_SOCKET){
		closesocket(m_sock);
		m_sock = EF_INVALID_SOCKET;
		m_sessid.clear();
	}
	
	return  0;	
}

int RPCClient::doRecv(int& cmd, std::string& msg, int timeout_ms){

	uint32	totallen = 0;
	int 	ret = 0;
	int 	rcvcnt = 0;
	int	actrcv = 0;


	msg.resize(12);
	char* buf = (char*)msg.data();

	ret = tcpReceive(m_sock, buf, 12, timeout_ms, &actrcv);
	if(ret < 12){
		return -1;
	}
	
	totallen = *(uint32*)(buf + 4);
	totallen = ntohl(totallen);
	cmd = ntohl(*(uint32*)(buf));
	rcvcnt = 12;

	msg.resize(totallen - 12);
	buf = (char*)msg.data();

	actrcv = 0;
	ret = tcpReceive(m_sock, buf, totallen - 12, timeout_ms, &actrcv);

	if(ret < (int)(totallen - 12)){
		return -2;
	}

	//std::cout << "msg.size:" << msg.size() << std::endl;
	return	totallen;

}

int RPCClient::doSend(int cmd, const std::string& req, int timeout_ms){
	std::string msg;
	head 	h;
	int	actsnd = 0;
	
	h.cmd = cmd;
	h.magic = 0x20140417;
	constructPacket(h, req, msg);

	int32 ret = 0;
	ret = tcpSend(m_sock, msg.data(), msg.size(), timeout_ms, &actsnd);

	return ret;
}


};
