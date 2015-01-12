#include "peer_conn.h"
#include "dispatcher.h"
#include "connect_server.pb.h"
#include "peer_server.pb.h"
#include "peer_cmd.h"
#include "err_no.h"
#include "msg_head.h"
#include "db_conn.h"
#include "settings.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "base/ef_base64.h"

namespace gim{


	int PeerCon::handleGetPeerMessage(const ServiceRequest& svreq,
			const PeerPacket& reqpk, PeerPacket& resppk){
		int ret = 0;
		TimeRecorder t("PeerCon::handleGetMessage");
		//construct resp
		resppk.set_cmd(GET_PEER_MESSAGE_RESP);
		GetPeerMessageResponse* gresp = resppk.mutable_get_peer_msg_resp();
		gresp->set_last_msgid(0);

		if(!reqpk.has_get_peer_msg_req()){
			FormatErrorLog(svreq.sn(), "get_peer_message");
			return INPUT_FORMAT_ERROR;	
		}	

		const GetPeerMessageRequest& gmsgreq = reqpk.get_peer_msg_req();
		//get message from db
		const string& cid = gmsgreq.cid();
		int64 start_msgid = gmsgreq.start_msgid();
		int64 count = gmsgreq.count();	
		int64 lastmsgid = 0;

		MsgDB* c = DBConn::getMsgDB();	

		if(!c){
			PeerErrorLog(svreq.sn(), "get_peer_message", cid, DB_ERROR);
			return DB_ERROR;	
		}

		int64 msgid = 0;

		//get last msg id
		ret = c->getMsgId("peer_" + cid + "_last_msgid", msgid);
		if(ret < 0){
			PeerErrorLog(svreq.sn(), "get_peer_message", cid, DB_ERROR);
			return DB_ERROR;
		}

		lastmsgid = msgid;

		//get last read
		if(start_msgid <= 0){
			msgid = 0;
			ret = c->getMsgId("peer_" + cid + "_last_read", msgid);

			if(ret < 0){
				PeerErrorLog(svreq.sn(), "get_peer_message", cid, DB_ERROR);
				return DB_ERROR;
			}

			if(msgid)
				start_msgid = msgid;
		}

		if(count <= 0){
			count = DEFAULT_MSG_COUNT; 
		}

		vector<Message> msgs;
		ret = c->getMsgs(msgs, "peer_" + cid, start_msgid, count, ORDER_BY_INCR);

		if(ret < 0){
			PeerErrorLog(svreq.sn(), "get_peer_message", cid, DB_ERROR);
			return DB_ERROR;
		}

		if(ret > 0)
			ret = 0;

		std::stringstream os;

		int64 lastread = start_msgid;

		//construct resp
		gresp->set_last_msgid(lastmsgid);

		for(size_t i = 0; i < msgs.size(); ++i){
			Message* pm = gresp->add_msgs();	
			pm->set_id(msgs[i].id());
			pm->set_to(cid);
			pm->set_time(msgs[i].time());
			pm->set_from(msgs[i].from());
			pm->set_data(msgs[i].data());
			pm->set_sn(msgs[i].sn());
			os << "<action:get_peer_message> <to:"
				<< cid << "> <from:" 
				<< msgs[i].from() << "> <msgid:"
				<< msgs[i].id() << "> <time:"
				<< msgs[i].time() << "> <msg_sn:"
				<< msgs[i].sn() << "\n";
			if(lastread < msgs[i].id())
				lastread = msgs[i].id();
		}

		c->setMsgId("peer_" + cid + "_last_read", 
				lastread);

		c->delMsgs("peer_" + cid, 
			start_msgid - MSG_LEFT_COUNT, -1);

		PLogTrace("PeerServer")
			<< "<action:handle_get_peer_message> <cid:" 
			<< cid << "> <start_msgid:" 
			<< start_msgid << "> <count:"
			<< count << "> <status:" << ret 
			<< "> <last_msgid:" << gresp->last_msgid() << "> <size:"
			<< gresp->msgs_size() << "> <errstr:" 
			<< getErrStr(ret) << ">";
		if(gresp->msgs_size())
			PLogTrace("PeerServer") << os.str();
		return ret;
	}

	int PeerCon::handleRecvPeerMessage(const ServiceResponse& svresp,
			const PeerPacket& reqpk){
		int ret = 0;
		TimeRecorder t("PeerCon::handleRecvMessage");

		if(!reqpk.has_recv_peer_msg_resp()){
			FormatErrorLog(svresp.sn(), "recv_peer_message");
			return INPUT_FORMAT_ERROR;	
		}	

		const RecvPeerMessageResponse& gmsgreq = reqpk.recv_peer_msg_resp();
		//get message from db
		const Message& msg = gmsgreq.msg();

		MsgDB* c = DBConn::getMsgDB();
		if(!c){
			PeerErrorLog(svresp.sn(), "recv_peer_message", msg.to(), DB_ERROR);
			return DB_ERROR;
		}

		c->delMsgs("peer_" + msg.to(), msg.id() - MSG_LEFT_COUNT, -1);

		ALogTrace("PeerServer")
			<< "<sn:" << svresp.sn()
			<< "> <action:recv_peer_message> <to:" 
			<< msg.to() << "> <msgid:" 
			<< msg.id() << "> <from:"
			<< msg.from() << "> <time:"
			<< msg.time() << "> <msg_sn:" 
			<< svresp.sn() << "> <errstr:" 
			<< getErrStr(ret) << ">";
		return ret;
	}


	int PeerCon::handleSendPeerMessage(const ServiceRequest& svreq,
			const PeerPacket& reqpk, PeerPacket& resppk){
		int ret = 0;

		TimeRecorder trcd("PeerCon::handleSendMessage");
		//construct resp
		resppk.set_cmd(SEND_PEER_MESSAGE_RESP);
		SendPeerMessageResponse* gresp = resppk.mutable_send_peer_msg_resp();	

		if(!reqpk.has_send_peer_msg_req()){
			FormatErrorLog(svreq.sn(), "recv_peer_message");
			return INPUT_FORMAT_ERROR;
		}
		const SendPeerMessageRequest& smsgreq = reqpk.send_peer_msg_req();
		const Message&  pm = smsgreq.msg();
		Message* resppm = gresp->mutable_msg();
		*(resppm) = pm;
		

		MsgDB* c = DBConn::getMsgDB();
		if(!c){
			PeerErrorLog(svreq.sn(), "send_peer_message", pm.to(), DB_ERROR);
			return DB_ERROR;		
		}

		int64 msgid = 0;
		ret = c->incrId("peer_" + pm.to() + "_last_msgid", msgid);
		if(ret < 0){
			PeerErrorLog(svreq.sn(), "send_peer_message", pm.to(), DB_ERROR);
			return DB_ERROR;
		}

		int64 t = gettime_ms();
		resppm->set_id(msgid);
		resppm->set_time(t);
		Message m;
		m.set_id(msgid);
		m.set_from(pm.from());
		m.set_time(t);
		m.set_data(pm.data());	
		m.set_sn(svreq.sn());
		if(ret < 0){
			PeerErrorLog(svreq.sn(), "send_peer_message", pm.to(), DB_ERROR);
			return DB_ERROR;
		}
	
		Settings *pSettings = Singleton<Settings>::instance();			

		//send to reciver 
		PeerPacket msgpk;
		msgpk = reqpk;
		SendPeerMessageRequest* spreq = msgpk.mutable_send_peer_msg_req();
		Message* pmsg = spreq->mutable_msg();
		pmsg->set_id(msgid);
		pmsg->set_time(t);
		if(ret > 0){
			ret = 0;
		}

		string payload;
		msgpk.SerializeToString(&payload);

		Dispatcher* d = getDispatcher();

		if(!d){
			PLogError("PeerServer")
				<< "<action:dispatch_peer_message> <from:"
				<< pm.from() << "> <to:" << pm.to() 
				<< "> <status:-1> <msgid:" << msgid 
				<< "> <errstr: get_dispatche_fail>";
			goto exit;
		}

		d->sendServiceRequest(getEventLoop(), 
				pm.to(), svreq.svtype(), 
				svreq.sn(), payload);

		if(pSettings->RespToMutiDevice){
			//when muti device online at the same time, 
			//should send the response
			//to all the device			
			string resppayload;
			resppk.SerializeToString(&resppayload);	
			d->sendServiceResponse(getEventLoop(), 
				pm.from(), svreq.svtype(), 
				svreq.sn(), ret, resppayload);
		}
	exit:
		//construct resp
		PLogTrace("PeerServer")
			<< "<action:send_peer_message> <to:" << pm.to()
			<< "> <from:" << pm.from() 
			<< "> <data:" << base64Encode(pm.data())
			<< "> <msg_sn:" << m.sn() << "> <status:" 
			<< ret << "> <msgid:" << msgid << "> <errstr:"
			<< getErrStr(ret) << ">";
		return ret;	
	}

	int PeerCon::handlePeerCmd(const ServiceRequest& svreq, string& resppayload){
		int ret = 0;
		PeerPacket reqpack;

		if(!reqpack.ParseFromString(svreq.payload())){
			FormatErrorLog(svreq.sn(), "handle_peer_cmd");
			return INPUT_FORMAT_ERROR;
		}

		PeerPacket resppack;
		Settings *pSettings = Singleton<Settings>::instance();			

		switch(reqpack.cmd()){
			case	SEND_PEER_MESSAGE_REQ:
				ret = handleSendPeerMessage(svreq, reqpack, resppack);
				if(!pSettings->RespToMutiDevice){
					resppack.SerializeToString(&resppayload);
				}
				break;
			case	GET_PEER_MESSAGE_REQ:
				ret = handleGetPeerMessage(svreq, reqpack, resppack);
				resppack.SerializeToString(&resppayload);	
				break;
			default:
				FormatErrorLog(svreq.sn(), "handle_peer_cmd");
				break;
		}

		return ret;
	}

	PeerConFac	g_peerconfac;

	SvConFactory* getPeerConFactory(){
		return &g_peerconfac;
	}

	int PeerCon::handleServiceRequest(const char* reqbody, int len,
			std::string& resp){
		int ret = 0;
		ServiceRequest svreq;
		if(!svreq.ParseFromArray(reqbody, len)){
			ALogError("PeerServer") << "<action:handle_cmd> <status:"
				<< (int)INPUT_FORMAT_ERROR
				<< "> <errstr:INPUT_FORMAT_ERROR>";
			return INPUT_FORMAT_ERROR;	
		}
		ServiceResponse svresp;
		svresp.set_sessid(svreq.sessid());
		svresp.set_svtype(svreq.svtype());
		svresp.set_sn(svreq.sn());
		string resppayload;
		switch(svreq.svtype()){
			case SERVICE_TYPE_PEER:
				ret = handlePeerCmd(svreq, resppayload);
				break;
			default:
				PLogError("PeerServer") 
					<< "<action:handle_cmd> <status:"
					<< (int)SERVICE_TYPE_ERROR 
					<< "> <errstr:SERVICE_TYPE_ERROR>";
				ret = SERVICE_TYPE_ERROR; 
		}
		if(resppayload.size()){
			svresp.set_status(ret);
			svresp.set_payload(resppayload);
			svresp.SerializeToString(&resp);
		}
		return 0;
	}

	int PeerCon::handleServiceResponse(const char* reqbody, int len){
		int ret = 0;
		ServiceResponse svresp;
		if(!svresp.ParseFromArray(reqbody, len)){
			ALogError("PeerServer") 
				<< "<action:handleServiceResponse> <status:"
				<< (int)INPUT_FORMAT_ERROR
				<< "> <errstr:INPUT_FORMAT_ERROR>";
			return 0;	
		}
		if(svresp.svtype() != SERVICE_TYPE_PEER){
			ALogError("PeerServer") 
				<< "<sn:" << svresp.sn()
				<< "> <action:handleServiceResponse> <status:"
				<< (int)SERVICE_TYPE_ERROR 
				<< "> <errstr:SERVICE_TYPE_ERROR>";
			return 0;
		}
		PeerPacket reqpk;
		if(!reqpk.ParseFromArray(svresp.payload().data(), 
			svresp.payload().size())){
			ALogError("PeerServer")
				<< "<sn:" << svresp.sn()
				<< "<action:handleServiceResponse> <status:"
				<< (int)(INPUT_FORMAT_ERROR)
				<< "> <errstr:INPUT_FORMAT_ERROR>";

		}
		ret = handleRecvPeerMessage(svresp, reqpk);
		return 0;
	}	
	
}
