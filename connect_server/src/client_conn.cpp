#include "client_conn.h"
#include "common.h"
#include "type_map.h"
#include "sess_cache.h"
#include "ef_crypt.h"
#include "connect_server.h"
#include "token_checker.h"
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

#define SVID m_serv->getConfig().ID

#define ALogTrace(a) logTrace(a) << "<svid:" << (SVID) \
	<< "> <addr:" << getIp() << ":" \
	<< getPort() << "> <timestamp:" << gettime_ms() << "> " 
#define ALogError(a) logError(a) << "<svid:" << (SVID) \
	<< "> <addr:" << getIp() << ":" \
	<< getPort() << "> <timestamp:" << gettime_ms() << "> " 



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


CliCon::~CliCon(){
	delSession();
	atomicDecrement32(&cnt);
}


volatile int CliCon::cnt = 0;

int CliCon::delSession(){
	TimeRecorder t(__FUNCTION__);

	time_t n = time(NULL);

	if(m_sess.type() == 0){
		return 0;
	}

	if(STATUS_LOGIN != m_status){
		return 0;
	}

	if(m_sess.type() > 0){
		TypeMap::delServer(m_sess.type(), this);
		return 0;
	}

	SessCache* c = SsChFactory::getSsChFactory()->getSessCache();		

	if(c){
		c->delSession(m_sess);
	}

	ALogError("ConnectServer") << "<action:client_logout> <id:" 
		<< m_sess.id() << "> <sessid:" << m_sess.sessid()
		<< "> <login_timestamp:" << m_login_time
		<< "> <online_time:" << n - m_login_time 
		<< ">";
	return 0;
}


int CliCon::onCreate(EventLoop* l){
	addNotify(l, READ_EVENT);	
	if(m_conf->AliveMs){
		startTimer(CHECK_TIMER, m_conf->AliveMs);
	}
	return 0;
}

int CliCon::handleTimer(EventLoop* l, int id){
	time_t n = time(NULL);
	switch(id){
	case CHECK_TIMER:
		if(n - m_sess.lasttime() > m_conf->AliveMs/1000){
			ALogError("ConnectServer") << "<id:" << m_sess.id() 
				<< "> <sessid:" << m_sess.sessid()
				<< "> <status:timeout>";
			safeClose();
			return 0;
		}else{
			updateSession();
		}
		startTimer(CHECK_TIMER, m_conf->AliveMs);
		break;
	}

	return 0;
}


int CliCon::updateSession(){
	TimeRecorder t(__FUNCTION__);

	time_t n = time(NULL);
	m_sess.set_lasttime(n);	

	//type is 0 or > 0, return
	if(m_sess.type() >= 0){
		return 0;
	}

	SessCache* c = SsChFactory::getSsChFactory()->getSessCache();		
	if(!c){
		return -1;
	}

	return c->setSession(m_sess);
}


int CliCon::checkToken(const string& token){
	TokenChecker* ck = m_serv->getTokenChecker();
	if(!ck){
		return INNER_ERROR;
	}

	int ret = 0;
	map<string, string> infos;	


	ret = ck->checkToken(token, infos);

	if(ret < 0){
		return ret;
	}	

	if(infos["uid"] != m_sess.id()){
		return -3;
	}


	return 0;
}

int CliCon::checkType(int type){
	//do not check rpc_client;
	if(!type)
		return 0;
	if(type < m_conf->MinType || type >= m_conf->MaxType){
		return -1;
	}
	return 0;
}

int CliCon::handleLoginRequest(const head& h, const string& req, 
	string& resp){
	TimeRecorder t("CliCon::handleLoginRequest");
	int ret = 0;
	int i = 0;
	LoginRequest lgreq;
	LoginResponse lgresp;	
	string tk;
	string sessid;
	string enctk;

	if(!lgreq.ParseFromArray(req.data()+sizeof(h), 
		req.size()-sizeof(h))){
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}
	//if login, return the sessid
	if(m_status == STATUS_LOGIN){
		goto exit;
	}

	m_login_time = time(NULL);	
	m_sess.set_id(lgreq.id());
	m_sess.set_type(lgreq.type());
	m_sess.set_consvid(m_serv->getConfig().ID);
	m_sess.set_version(lgreq.version());

	if(m_conf->Enc && !lgreq.has_token()){

		ret = CHECK_TOKEN_FAIL;
		goto exit;
	}

	if(m_conf->Enc){
		enctk = lgreq.token();

		ret = ef::decrypt(enctk, tk);

		if(ret < 0){
			return ret;
		}

		//check token
		ret = checkToken(tk);

		if(ret < 0){
			ret = CHECK_TOKEN_FAIL;
			goto exit; 

		}

	}

	ret = checkType(lgreq.type());

	if(ret < 0){
		ret = INVALID_TYPE;
		goto exit;
	}

	decorationName(m_serv->getConfig().ID, getId(), lgreq.id(), sessid);
	m_sess.set_sessid(sessid);

	m_key = tk + sessid;


	for(; i < lgreq.kvs_size(); ++i){
		*(m_sess.add_kvs()) = lgreq.kvs(i);
	}

	if(lgreq.type() > 0){
		TypeMap::addServer(lgreq.type(), this);
	}
	
	ret = updateSession();

	if(ret < 0){
		ret = CREATE_SESSION_FAIL;
	}
exit:
	if(ret < 0){
		lgresp.set_status(ret);
	}else{
		m_status = STATUS_LOGIN;
		lgresp.set_status(0);
	}

	lgresp.set_sessid(m_sess.sessid());

	lgresp.SerializeToString(&resp);
	ALogError("ConnectServer") << "<action:client_login> <id:" 
		<< m_sess.id() << "> <version:" << m_sess.version() 
		<< "> <type:" << lgreq.type()
		<< "> <token:" << lgreq.token()  
		<< "> <sessid:" << m_sess.sessid() << "> <status:"
		<< ret << "> <errstr:" << getErrStr(ret) << ">";
	return ret;
}


int CliCon::checkReqQue(int reqcnt, int respcnt){
	m_req_cnt += reqcnt;
	m_resp_cnt += respcnt;
	if(m_sess.type() <= 0 || m_conf->MaxReqQueSize <= 0){
		return 0;
	}
	if(m_req_cnt - m_resp_cnt >= m_conf->MaxReqQueSize){
		TypeMap::delServer(m_sess.type(), this);
		m_busy = true;
	}

	if(m_req_cnt - m_resp_cnt < m_conf->MaxReqQueSize / 2 && m_busy){
		TypeMap::addServer(m_sess.type(), this);
		m_busy = false;
	}

	return 0;
}

int CliCon::getConFromSessid(const string& sessid, 
	EventLoop*& l, int& conid, string& orgdata){
	int ret = 0;
	int svid = 0;
	int evlpid = 0;

	ret = getDecorationInfo(sessid, svid, conid, orgdata);

	if(ret < 0 || svid != m_serv->getConfig().ID){
		ret = INVALID_SESSION_ID;
		goto exit;	
	}

	evlpid = getEventLoopId(conid);
	
	if(evlpid < 0 || evlpid >= m_serv->getConfig().ThreadCount){
		ret = INVALID_SESSION_ID;
		goto exit;
	}

	l = &(m_serv->getEventLoop(evlpid));

exit:
	return ret;
}

int CliCon::sendCmd(int cmd, const string& body){
	string req_enc;
	string msg;
	if((cmd == SERVICE_REQ || cmd == SERVICE_RESP) 
		&& body.size()){
		int ret = encodeBody(body.data(), body.size(), req_enc);
		if(ret < 0){
			return ret;
		}
	}else{
		req_enc = body;
	}
	head h;
	h.cmd = cmd;
	h.magic = MAGIC_NUMBER;
	constructPacket(h, req_enc, msg);
	return sendMessage(msg);
}

int CliCon::decodeBody(const char* encbody, int bodylen, string& body){
	int ret = 0;
	if(m_conf->Enc){
		//string hexbody;
		//hexbody.resize(bodylen * 2);
		//bytesToHexs(encbody, bodylen, 
		//	(char*)hexbody.data(), hexbody.size());
		ret = aesDecrypt(encbody, 
			bodylen, m_key, body);	
	}else{
		body.append(encbody, bodylen);
	}
	return ret;
}


int CliCon::encodeBody(const char* body, int bodylen, string& encbody){
	int ret = 0;
	if(m_conf->Enc){
		ret = aesEncrypt(body, 
			bodylen, m_key, encbody);	
	}else{
		encbody.append(body, bodylen);
	}
	return ret;
}

int CliCon::handleServiceRequest(const head& h, const string& req, 
	string& resp){
	TimeRecorder t("CliCon::handleServiceRequest");

	int ret = 0;
	int conid = 0;

	EventLoop* l = NULL;
	NetOperator* op = NULL;

	ServiceResponse svresp;
	ServiceRequest svreq;
	string newreq;
	//decode req
	string req_dec;
	string key;
	string id;

	svresp.set_from_sessid("NULL");
	svresp.set_to_sessid("NULL");
	svresp.set_from_type(0);
	svresp.set_to_type(0);
	svresp.set_sn("null");

	ret = decodeBody(req.data() + sizeof(h), req.size() - sizeof(h), req_dec);

	if(ret < 0){
		ret = DECRYPT_FAIL;
		goto exit;
	}

	if(!svreq.ParseFromArray(req_dec.data(), 
		req_dec.size())){
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}

	svresp.set_from_type(m_sess.type());
	svresp.set_to_type(svreq.from_type());
	svresp.set_sn(svreq.sn());
	svresp.set_status(0);

	//from rpc call,do not check sessid;
	if(m_sess.type() && svreq.from_sessid() != m_sess.sessid()){
		ret = INVALID_SESSION_ID;	
		goto exit;
	}



	svreq.set_from_sessid(m_sess.sessid());
	svresp.set_to_sessid(svreq.from_sessid());

	if(!svreq.has_to_sessid()){
		ret = TypeMap::transRequest(svreq);
		ALogError("ConnectServer")
			<< "<action:trance_service_request> <from_sessid:"
			<< svreq.from_sessid() << "> <key:" << key
			<< "> <sn:" << svreq.sn() << "> <from_type:"
			<< svreq.from_type() << "> <to_type:" << svreq.to_type()
			<< "> <status:" << ret << ">"; 
		if(ret < 0){
			svresp.set_status(ret);
			ret = 0;
		}
		goto exit;	
	}


	ret = getConFromSessid(svreq.to_sessid(), l, conid, id);
	if(ret < 0){
		svresp.set_status(ret);
		ret = 0;
		goto exit;
	}

	svreq.SerializeToString(&newreq);
	op = new SendMsgOp(conid, SERVICE_REQ, newreq);	
	ret = l->addAsynOperator(op);
	if(ret < 0){
		svresp.set_status(ret);
		ret = 0;
		goto exit;
	}

exit:
	if(ret < 0){
		svresp.set_status(ret);
	}

	if(svresp.status()){
		svresp.SerializeToString(&resp);
	}

	ALogError("ConnectServer") << "<action:service_request> <id:" 
		<< m_sess.id() << "> <version:" << m_sess.version() 
		<< "> <sessid:" << m_sess.sessid() << "> <from_sessid:"
		<< svreq.from_sessid() << "> <from_type:"
		<< svreq.from_type() << "> <sn:" << svresp.sn()
		<< "> <to_sessid:" << svreq.to_sessid()
		<< "> <to_type:" << svreq.to_type()
		<< "> <conid:" << conid 
		<< "> <status:" << svresp.status() << "> <errstr:"
		<< getErrStr(svresp.status()) << ">";
	return ret;
}

int32 CliCon::handleServiceResponse(const head& h, 
		const string& req){
	TimeRecorder t("SrvCon::handleServiceResponse");
	int32 ret = 0;
	int32 conid = 0;
	string id; 
	string req_dec;
	ServiceResponse svresp;
	EventLoop* l = NULL;
	SendMsgOp* op = NULL;


	ret = decodeBody(req.data() + sizeof(h), req.size() - sizeof(h), req_dec);

	if(ret < 0){
		ret = DECRYPT_FAIL;
		goto exit;
	}


	if(!svresp.ParseFromArray(req_dec.data(), req_dec.size())){
		ret = INPUT_FORMAT_ERROR;	
		goto exit;
	}


	if(!svresp.has_to_sessid() || !svresp.to_sessid().size()){
		ret = MISS_TO_SESSION_ID;
		goto exit;
	}

	//cout << "RESP_TO_SESSID:" << svresp.to_sessid() << endl;

	ret = getConFromSessid(svresp.to_sessid(), l, conid, id);
	if(ret < 0){
		goto exit;
	}
	//???
	op = new SendMsgOp(conid, SERVICE_RESP, req_dec);
	ret = l->addAsynOperator(op);
exit:
	ALogError("ConnectServer")  
		<< "<action:service_response> <sessid:"
		<< m_sess.sessid() << "> <from_sessid:" 
		<< svresp.from_sessid() << "> <to_sessid:"
		<< svresp.to_sessid() << "> <from_type:"
		<< svresp.from_type() << "> <to_type:" 
		<< svresp.to_type() << "> <sn:" 
		<< svresp.sn() << "> <status:" << ret
		<< "> <errstr:" << getErrStr(ret) << ">";
	if(ret != INPUT_FORMAT_ERROR)
		ret = 0;
	return ret;
}

Connection* CliConFactory::createConnection(EventLoop* l,
		int fd, const string& addr, int port){
	Connection* c = new CliCon(m_serv, m_conf);
	return c;
}



int CliCon::kickClient(const string& reason){
	head h;
	h.cmd = KICK_CLIENT;
	KickCliRequest kr;
	kr.set_reason(reason);
	string body;
	kr.SerializeToString(&body);		 
	string msg;
	constructPacket(h, body, msg);
	sendMessage(msg);
	ALogError("ConnectServer") << "<id:" << m_sess.id()
		<< "> <version:" << m_sess.version() 
		<< "> <sessid:" << m_sess.sessid()
		<< "> <status:been_kicked> <reason:" 
		<< reason << ">";
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
			ret = handleLoginRequest(h, req, resp);		
			break;
		case SERVICE_REQ:
			updateSession();
			ret = handleServiceRequest(h, req, resp);
			break;
		case SERVICE_RESP:
			updateSession();
			ret = handleServiceResponse(h, req);
			break;
		case KEEPALIVE_REQ:
			updateSession();
			break;	
		default:
			ALogError("ConnectServer") << "<id:" << m_sess.id()
				<< "> <sessid:" << m_sess.sessid() 
				<< "> <status:unknow_cmd>";
			ret = -1;
		}
		if(resp.size() || h.cmd == KEEPALIVE_REQ){	
			ret = sendCmd(h.cmd + 1, resp);
		}
	}catch(exception& e){
		ALogError("ConnectServer") << "<action:client_cmd:" << h.cmd << "> <id:" 
			<< m_sess.id() << "> <version:" << m_sess.version() 
			<< "> <sessid:" << m_sess.sessid() << "> <conid:" 
			<< getId() << "> <status:exception> <what:" << e.what()
			<< ">";
		ret = -1;
	}catch(...){
		ALogError("ConnectServer") << "<action:client_cmd>" << h.cmd << " <id:" 
			<< m_sess.id() << "> <version:" << m_sess.version() 
			<< "> <sessid:" << m_sess.sessid() << "> <conid:" 
			<< getId() << "> <status:exception>";
		ret = -1;
	}
	return ret;
}


int CliCon::checkPackLen(){
	int ret = 0;
	ret = checkLen(*this);
	if(ret <= 0 && bufLen() > 0){
		ALogError("ConnectServer") << "<action:client_check_pack> <id:" 
			<< m_sess.id() << "> <sessid:" << m_sess.sessid() 
			<< "> <conid:" << getId() << "> <buflen:" << bufLen() 
			<< "> <status:packet_not_full>";

	}
	return ret;
}


int SendMsgOp::process(ef::EventLoop *l){
	CliCon* clic = (CliCon*)l->getConnection(m_conid);
	if(!clic){
		return -1;
	}
	return clic->sendCmd(m_cmd, m_body);
}

}
