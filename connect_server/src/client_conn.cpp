#include "client_conn.h"
#include "proto/msg_head.h"
#include "proto/err_no.h"
#include "proto/connect_server.pb.h"
#include "base/ef_base64.h"
#include "base/ef_aes.h"
#include "base/ef_md5.h"
#include "base/ef_hex.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "common.h"
#include "server_manager.h"

namespace gim{


int32 CDispatcher::dispatchConnection(EventLoop*l, Connection* c){
	int32 id = l->getConId();
	int32 idx = 0;	
	int32 evlpcnt = m_s->getEventLoopCount();

	if(evlpcnt > 1){
		int32 svevcnt = serverEventLoopCount(evlpcnt); 
		int32 clievcnt = evlpcnt - svevcnt;
		idx = svevcnt + id % clievcnt;	
	}
		
	EventLoop& lp = m_s->getEventLoop(idx);
	id = getConnectionId(idx, id);
	c->setId(id);
	lp.asynAddConnection(id, c);
	return 0;		
}


CliCon::~CliCon(){
	delSession();
	atomicDecrement32(&cnt);
}


volatile int32 CliCon::cnt = 0;

int32 CliCon::delSession(){
	TimeRecorder t(__FUNCTION__);
	time_t n = time(NULL);

	if(STATUS_LOGIN != m_status){
		return 0;
	}

	SessCache* c = CacheConn::getSessCache();		

	if(c){
		c->delSession(m_sess);
	}

	ALogError("ConnectServer") << "<action:client_logout> <cid:" 
		<< m_sess.cid() << "> <sessid:" << m_sess.sessid()
		<< "> <login_timestamp:" << m_login_time
		<< "> <online_time:" << n - m_login_time 
		<< ">";
	return 0;
}


int32 CliCon::onCreate(EventLoop* l){
	addNotify(l, READ_EVENT);	
	if(m_alive_ms){
		startTimer(CHECK_TIMER, m_alive_ms);
	}
	return 0;
}

int32 CliCon::handleTimer(EventLoop* l, int32 id){
	time_t n = time(NULL);
	switch(id){
	case CHECK_TIMER:
		if(n - m_sess.lasttime() > m_alive_ms/1000){
			ALogError("ConnectServer") << "<cid:" << m_sess.cid() 
				<< "> <sessid:" << m_sess.sessid()
				<< "> <status:timeout>";
			safeClose();
			return 0;
		}else{
			updateSession();
		}
		startTimer(CHECK_TIMER, m_alive_ms);
		break;
	}

	return 0;
}


int32 CliCon::updateSession(){
	TimeRecorder t(__FUNCTION__);
	SessCache* c = CacheConn::getSessCache();		
	if(!c){
		return -1;
	}

	return c->setSession(m_sess);
}


int32 CliCon::checkTime(int32 max_dif){
	if(m_login_time > m_client_login_time + max_dif
		|| m_client_login_time > m_login_time + max_dif){
		return -1;
	}
	return 0;
}

int32 CliCon::checkToken(const std::string& token){
	UserDB* udb = CacheConn::getUserDB();

	if(!udb){
		return GET_USER_KEY_FAIL;
	}

	map<string, string> pro;

	int32 ret = udb->getUserInfo(m_sess.cid(),  pro);	

	if(ret < 0){
		return GET_USER_KEY_FAIL;
	}

	m_key = pro[USER_KEY_FIELD_NAME];

	std::string tk;
	std::string s = m_sess.cid() + m_key + m_version + 
		itostr(m_client_login_time);

	MD5Hex(tk, (const uint8*)s.data(), s.size());

	if(tk != token){
		return CHECK_TOKEN_FAIL;
	}

	return 0;
}

int32 CliCon::handleLoginRequest(const head& h, const std::string& req, 
	std::string& resp){
	TimeRecorder t("CliCon::handleLoginRequest");
	int32 ret = 0;
	int32 i = 0;
	LoginRequest lgreq;
	LoginResponse lgresp;	
	std::string tk;
	std::string sessid;

	Settings *pSettings = Singleton<Settings>::instance();

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
	m_sess.set_cid(lgreq.cid());
	m_sess.set_svid(pSettings->Id);
	m_version = lgreq.version();
	m_enc = lgreq.enc();
	m_client_login_time = lgreq.time();

	if(lgreq.enc()){

		ret = checkTime(pSettings->MaxTimeDif);
		if(ret < 0){
			return setClientTime();
		}	

		if(!lgreq.has_token()){
			ret = CHECK_TOKEN_FAIL;
			goto exit;
		}

		tk = lgreq.token();
		//check token
		ret = checkToken(tk);

		if(ret < 0){
			goto exit;
		}
	}else{
		
		if(!pSettings->NoEncSupport){
			ret = CHECK_TOKEN_FAIL;
			goto exit; 
		}

	}

	decorationName(pSettings->Id, getId(), lgreq.cid(), sessid);
	m_sess.set_sessid(sessid);

	for(; i < lgreq.kvs_size(); ++i){
		*(m_sess.add_kvs()) = lgreq.kvs(i);
	}

	ret = updateSession();

exit:
	if(ret < 0){
		lgresp.set_status(ret);
	}else{
		m_status = STATUS_LOGIN;
		lgresp.set_status(0);
	}

	lgresp.set_sessid(m_sess.sessid());

	lgresp.SerializeToString(&resp);
	ALogError("ConnectServer") << "<action:client_login> <cid:" 
		<< m_sess.cid() << "> <version:" << m_version 
		<< "> <enc:" << lgreq.enc()
		<< "> <token:" << lgreq.token() << getSessKVsString()
		<< "> <sessid:" << m_sess.sessid() << "> <status:"
		<< ret << "> <errstr:" << getErrStr(ret) << ">";
	return ret;
}

std::string CliCon::getSessKVsString(){
	std::stringstream os;
	for(int32 i = 0; i < m_sess.kvs_size(); ++i){
		os << " <" << m_sess.kvs(i).key() << ":"
			<< m_sess.kvs(i).value() << ">";
	}
	return os.str();
}

int32 CliCon::getServConFromSN(const std::string& sn, 
	EventLoop*& l, int32& conid, std::string& oldsn){
	int32 ret = 0;
	int32 svid = 0;
	int32 evlpid = 0;

	Settings* setting = Singleton<Settings>::instance();
	ret = getDecorationInfo(sn, svid, conid, oldsn);

	if(ret < 0 || svid != setting->Id){
		ret = INVALID_SN;
		goto exit;	
	}

	evlpid = getEventLoopId(conid);
	
	if(evlpid >= serverEventLoopCount(m_serv->getEventLoopCount())){
		ret = INVALID_SN;
		goto exit;
	}

	l = &(m_serv->getEventLoop(evlpid));

exit:
	return ret;
}

int32 CliCon::handleServiceResponse(const head& h, const std::string& req){
	TimeRecorder t("CliCon::handleServiceResponse");
	int32 ret = 0;
	int32 conid;
	ServiceResponse svresp;
	ServiceResponse oldsvresp;
	EventLoop* l = NULL;
	std::string oldsn;
	std::string oldresp;
	std::string respbuf;
	std::string req_dec;
	ret = decodeBody(req.data() + sizeof(h),
		req.size() - sizeof(h), req_dec);
	if(ret < 0){
		ret = DECRYPT_FAIL;
		goto exit;		
	}

	if(!svresp.ParseFromArray(req_dec.data(), 
		req_dec.size()))
	{
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}
	
	ret = getServConFromSN(svresp.sn(), l, conid, oldsn);
	if(ret < 0){
		goto exit;
	}

	oldsvresp = svresp;
	oldsvresp.set_sn(oldsn);
	oldsvresp.SerializeToString(&oldresp);
	constructPacket(h, oldresp, respbuf);
	if(l != getEventLoop()){
		ret = l->asynSendMessage(conid, respbuf);
	}else{
		ret = l->sendMessage(conid, respbuf);
	}
exit:
	ALogError("ConnectServer") << "<action:client_service_response> <cid:" 
		<< m_sess.cid() << "> <version:" << m_version 
		<< "> <sessid:" << m_sess.sessid() << "> <svtype:"
		<< svresp.svtype() << "> <sn:" << svresp.sn()
		<< "> <event_loop:" << l << "> <conid:" << conid
		<< "> <status:" << ret  << "> <errstr:"
		<< getErrStr(ret) << ">";
	if(ret != INPUT_FORMAT_ERROR)
		ret = 0;
	return ret;
}

int32 CliCon::sendToClient(int32 cmd, const std::string& body){
	std::string req_enc;
	std::string msg;
	if((cmd == SERVICE_REQ || cmd == SERVICE_RESP) 
		&& body.size()){
		int32 ret = encodeBody(body.data(), body.size(), req_enc);
		if(ret < 0)
			return ret;
	}else{
		req_enc = body;
	}
	head h;
	h.cmd = cmd;
	h.magic = MAGIC_NUMBER;
	constructPacket(h, req_enc, msg);
	return sendMessage(msg);
}

int32 CliCon::decodeBody(const char* encbody, int32 bodylen, std::string& body){
	int32 ret = 0;
	if(m_enc){
		std::string hexbody;
		hexbody.resize(bodylen * 2);
		bytesToHexs(encbody, bodylen, 
			(char*)hexbody.data(), hexbody.size());
		ret = aesDecrypt(encbody, 
			bodylen, m_key, body);	
	}else{
		body.append(encbody, bodylen);
	}
	return ret;
}


int32 CliCon::encodeBody(const char* body, int32 bodylen, std::string& encbody){
	int32 ret = 0;
	if(m_enc){
		ret = aesEncrypt(body, 
			bodylen, m_key, encbody);	
	}else{
		encbody.append(body, bodylen);
	}
	return ret;
}

int32 CliCon::handleServiceRequest(const head& h, const std::string& req, 
	std::string& resp){
	TimeRecorder t("CliCon::handleServiceRequest");
	int32 ret = 0;
	int32 ret1 = 0;
	int32 conid = 0;
	EventLoop* l = NULL;
	ServiceResponse sresp;
	ServiceRequest sreq;
	std::string treq;
	//decode req
	std::string req_dec;


	sresp.set_sessid("null");
	sresp.set_svtype(0);
	sresp.set_sn("null");

	ret = decodeBody(req.data() + sizeof(h), req.size() - sizeof(h), req_dec);

	if(ret < 0){
		ret = DECRYPT_FAIL;
		goto exit;
	}

	if(!sreq.ParseFromArray(req_dec.data(), 
		req_dec.size())){
		ret = INPUT_FORMAT_ERROR;
		goto exit;
	}


	sresp.set_svtype(sreq.svtype());
	sresp.set_sn(sreq.sn());
	sresp.set_status(0);


	if(sreq.sessid() != sessId()){
		ret = INVLID_SESSION_ID;	
		goto exit;
	}

	sresp.set_sessid(sreq.sessid());
	constructPacket(h, req_dec, treq);
	ret1 = getServMan().dispatchRequest(sreq.svtype(), 
		treq, getEventLoop(), l, conid);
exit:
	if(ret < 0){
		sresp.set_status(ret);
	}

	if(ret1 < 0){
		sresp.set_status(ret1);
	}

	if(sresp.status()){
		sresp.SerializeToString(&resp);
	}

	ALogError("ConnectServer") << "<action:client_service_request> <cid:" 
		<< m_sess.cid() << "> <version:" << m_version  
		<< "> <sessid:" << m_sess.sessid() << "> <svtype:"
		<< sresp.svtype() << "> <sn:" << sresp.sn()
		<< "> <conid:" << conid 
		<< "> <status:" << sresp.status() << "> <errstr:"
		<< getErrStr(sresp.status()) << ">";
	return ret;
}

Connection* CliConFactory::createConnection(EventLoop* l,
		int32 fd, const std::string& addr, int32 port){
	Settings *pSettings = Singleton<Settings>::instance();
	if(pSettings->MaxCliCount > 0 && 
		CliCon::totalCount() >= pSettings->MaxCliCount){
		logError("ConnectServer") <<  "addr:[" << addr << ":"
			<< port << "] <timestamp:" << gettime_ms()
			<< "> <action:accept_refuse> <fd:"
			<< fd << "> <status:-200000>"; 
		return NULL;
	}
	if(acceptControl() < 0){
		logError("ConnectServer") << "addr:[" << addr << ":" 
			<< port << "] <timestamp:" << gettime_ms()
			<< "> <action:accept_refuse> <fd:"
			<< fd << "> <status:-300000>";	
		return NULL;	
	}
	Connection* c = new CliCon(m_serv, m_con_alive_ms);
	return c;
}



int32 CliConFactory::acceptControl(){
	int32 ret = 0;
	Settings *pSettings = Singleton<Settings>::instance();

	if(pSettings->MaxAcceptSpeed <= 0){
		return 0;
	}

	time_t n = time(NULL);
	int32 c = atomicIncrement32(&m_cnt_one_cycle);
	time_t ct = m_check_time;

	if(n > ct){
		if(c > pSettings->MaxAcceptSpeed / (n - ct))
			ret = -1;
		m_cnt_one_cycle = 0;
		m_check_time = n;
	}else{
		if(c > pSettings->MaxAcceptSpeed){
			ret = -1;
		}
	}
	
	return ret;

}

int32 CliCon::frequencyControl(){

	Settings *pSettings = Singleton<Settings>::instance();
	m_sess.set_lasttime(time(NULL));

	if(pSettings->MaxReqFrequency <= 0){
		return 0;
	}

	time_t n = m_sess.lasttime();
	time_t diff = n - m_last_check_time;
	++m_pack_cnt_one_cycle;

	if(diff > FRE_CHECK_SPAN){
		if(m_pack_cnt_one_cycle * FRE_CHECK_SPAN 
			> pSettings->MaxReqFrequency * diff)
			return -1;
		m_pack_cnt_one_cycle = 0;
		m_last_check_time = n;
	}

	if(m_pack_cnt_one_cycle > pSettings->MaxReqFrequency)
		return -1;
	return 0;
}

int32 CliCon::setClientTime(){
	SetTimeResponse stresp;
	stresp.set_status(0);
	stresp.set_server_time(m_login_time);		
	std::string body;
	stresp.SerializeToString(&body);
	int ret = sendToClient(SET_TIME_RESP, body);
	ALogError("ConnectServer") << "<cid:" << m_sess.cid()
		<< "> <version:" << m_version << "> <client_time:"
		<< m_client_login_time << "> <server_time:"
		<< m_login_time << "> <status:reset_time>";
	return ret;
}

int32 CliCon::kickClient(const std::string& reason){
	head h;
	h.cmd = KICK_CLIENT;
	KickCliRequest kr;
	kr.set_reason(reason);
	std::string body;
	kr.SerializeToString(&body);		 
	std::string msg;
	constructPacket(h, body, msg);
	sendMessage(msg);
	ALogError("ConnectServer") << "<cid:" << m_sess.cid()
		<< "> <version:" << m_version 
		<< "> <sessid:" << m_sess.sessid()
		<< "> <status:been_kicked> <reason:" 
		<< reason << ">";
	return 0;	
}

int32 CliCon::handlePacket(const std::string& req){
	int32 ret = 0;
	head h = *(head*)req.data();
	h.cmd = htonl(h.cmd);
	h.len = htonl(h.len);
	ret = frequencyControl();
	if(ret < 0){
		kickClient("request frequency to high");
		return ret;
	}
	std::string resp;
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
			ALogError("ConnectServer") << "<cid:" << m_sess.cid()
				<< "> <sessid:" << m_sess.sessid() 
				<< "> <status:unknow_cmd>";
			ret = -1;
		}
		if(resp.size() || h.cmd == KEEPALIVE_REQ){	
			ret = sendToClient(h.cmd + 1, resp);
		}
	}catch(...){
		ALogError("ConnectServer") << "<action:client_cmd> <cid:" 
			<< m_sess.cid() << "> <version:" << m_version 
			<< "> <sessid:" << m_sess.sessid() << "> <conid:" 
			<< getId() << "> <status:exception>";
		ret = -1;
	}
	return ret;
}


int32 CliCon::checkPackLen(){
	int32 ret = 0;
	ret = checkLen(*this);
	if(ret <= 0 && bufLen() > 0){
		ALogError("ConnectServer") << "<action:client_check_pack> <cid:" 
			<< m_sess.cid() << "> <sessid:" << m_sess.sessid() 
			<< "> <conid:" << getId() << "> <buflen:" << bufLen() 
			<< "> <status:packet_not_full>";

	}
	return ret;
}

}
