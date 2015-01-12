#include "push_conn.h"
#include "dispatcher.h"
#include "peer_cmd.h"
#include "err_no.h"
#include "msg_head.h"
#include "db_conn.h"
#include "settings.h"
#include "logic_common.h"
#include "logic_server.h"
#include "peer_server.pb.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "base/ef_base64.h"

namespace gim{

#define DEFAULT_MSG_COUNT (5)
	
		
	int PushCon::onCreate(EventLoop* l){
		addNotify(l, READ_EVENT);
		return 0;
	}	

	int PushCon::handlePacket(const std::string& req){
		int ret = 0;
		head h = *(head*)req.data();
		h.cmd = htonl(h.cmd);
		h.len = htonl(h.len);	
		std::string resp;
		try{
			switch(h.cmd){
			case SERVICE_REQ:
				ret = handlePushMessage(h, 
					req.substr(sizeof(h)), resp);
				break;
			}
			if(resp.size()){
				std::string respbuf;
				head resph = h;
				resph.cmd = h.cmd + 1;
				constructPacket(h, resp, respbuf);
				sendMessage(respbuf);
			}	
		}catch(...){
			ALogError("PeerServer") << "<action:handle_push_packet>"
				" <status:exception>";
			ret = -1;
		}
		return ret;
	}
	

	int PushCon::handlePushMessage(const head& h, const string& req, string& resp){
		int ret = 0;
		TimeRecorder t("PushCon::handlePushMessage");
		
		PushMessageRequest svreq;
		PushMessageResponse svresp;
		svresp.set_sn("");

		Message m;
		int64 msgid = 0;

		do{

			if(!svreq.ParseFromString(req)){
				ret = INPUT_FORMAT_ERROR;
				break;
			}

			svresp.set_sn(svreq.sn());
			const Message&  pm = svreq.msg();
			const string& cid = pm.to();

			MsgDB* c = DBConn::getMsgDB();

			if(!c){
				PeerErrorLog(svreq.sn(), "push_message", cid, DB_ERROR);
				ret = DB_ERROR;
				break;
			}


			ret = c->incrId("peer_" + cid + "_last_msgid", msgid);
			if(ret < 0){
				PeerErrorLog(svreq.sn(), "push_message", cid, DB_ERROR);
				ret = DB_ERROR;
				break;
			}

			int64 tm = gettime_ms();
			m = pm;
			m.set_time(tm);
			m.set_sn(svreq.sn());


			ret = c->addMsg("peer_" + pm.to(), m);

			if(ret < 0){
				PeerErrorLog(svreq.sn(), "push_message", cid, DB_ERROR);
				ret = DB_ERROR;
				break;
			}
			
			//send to reciver 
			PeerPacket msgpk;
			msgpk.set_cmd(SEND_PEER_MESSAGE_REQ);
			SendPeerMessageRequest* spreq = msgpk.mutable_send_peer_msg_req();
			Message* pmsg = spreq->mutable_msg();
			*pmsg = pm; 
			pmsg->set_id(msgid);
			pmsg->set_time(tm);
			string payload;
			msgpk.SerializeToString(&payload);
			
			Dispatcher* d = getDispatcher();

			if(!d){
				PLogError("PeerServer")
					<< "<action:push_message> <from:"
					<< pm.from() << "> <to:" << pm.to() 
					<< "> <status:-1> <msgid:" << msgid 
					<< "> <errstr: get_dispatche_fail>";
				break;
			}

			d->sendServiceRequest(getEventLoop(), 
					pm.to(), SERVICE_TYPE_PEER, 
					svreq.sn(), payload);

		} while (0);
	
		if(ret > 0)
			ret = 0;

		svresp.set_status(ret);
		svresp.SerializeToString(&resp);
		//construct resp
		PLogTrace("PeerServer")
			<< "<action:push_message> <from:" 
			<< m.from() << "> <to:" << m.to() 
			<< "> <type:" << m.type() << "> <data:" 
			<< base64Encode(m.data()) << "> <status:" 
			<< ret << "> <msgid:" << msgid << "> <errstr:"
			<< getErrStr(ret) << ">";
		return ret;	
	}

	Dispatcher* PushCon::getDispatcher(){
		return m_serv->getDispatcher();
	}

	int PushCon::checkPackLen(){
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

	PushConFac	g_pushconfac;

	PushConFac* getPushConFactory(){
		return &g_pushconfac;
	}


}
