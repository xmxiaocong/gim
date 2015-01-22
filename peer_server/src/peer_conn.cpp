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


	int PeerCon::getPeerMsg(const std::string& cid, int64 start_msgid,
		int64 count, vector<Message>& msgs, int64& msgid){

		int ret= 0;

		MsgDB* c = DBConn::getMsgDB();	

		if(!c){
			return DB_ERROR;	
		}


		//get last msg id
		ret = c->getMsgId("peer_" + cid + "_last_msgid", msgid);
		if(ret < 0){
			return DB_ERROR;
		}

		int64 lastread = 0;

		//get last read
		if(start_msgid <= 0){
			ret = c->getMsgId("peer_" + cid + "_last_read", lastread);

			if(ret < 0){
				return DB_ERROR;
			}

			if(lastread)
				start_msgid = lastread;
		}

		if(count <= 0){
			count = DEFAULT_MSG_COUNT; 
		}

		ret = c->getMsgs(msgs, "peer_" + cid, start_msgid, count, ORDER_BY_INCR);

		if(ret < 0){
			return DB_ERROR;
		}

		if(!msgs.size()){
			return 0;
		}

		if(lastread < msgs.back().id()){
			lastread = msgs.back().id();

			c->setMsgId("peer_" + cid + "_last_read", 
				lastread);

			c->delMsgs("peer_" + cid, 
				start_msgid - MSG_LEFT_COUNT, -1);
		}

		return ret; 

	}

	int PeerCon::sendPeerMsg(const Message& pm, int64& msgid){

		int ret = 0;

		MsgDB* c = DBConn::getMsgDB();
		if(!c){
			return DB_ERROR;		
		}

		Message m = pm;

		ret = c->incrId("peer_" + pm.to() + "_last_msgid", msgid);
		if(ret < 0){
			return DB_ERROR;
		}

		m.set_id(msgid);
		m.set_time(gettime_ms());
		m.set_sn(getPackSN());
	
		ret = c->addMsg("peer_" + m.to(), m);	

		return ret; 

	}

	int PeerCon::handleGetPeerMessage(const PeerPacket& reqpk){
		int ret = 0;
		TimeRecorder t("PeerCon::handleGetMessage");
		//construct resp
		PeerPacket resppk;
		resppk.set_cmd(GET_PEER_MESSAGE_RESP);
		GetPeerMessageResponse* gresp = resppk.mutable_get_peer_msg_resp();
	
		const GetPeerMessageRequest& gmsgreq = reqpk.get_peer_msg_req();
		//get message from db
		const string& cid = gmsgreq.cid();
		int64 start_msgid = gmsgreq.start_msgid();
		int64 count = gmsgreq.count();	
		int64 lastmsgid = 0;
		vector<Message> msgs;

		std::stringstream os;
	
		gresp->set_last_msgid(0);

		if(!reqpk.has_get_peer_msg_req()){
			FormatErrorLog(getPackSN(), "get_peer_message");
			ret = INPUT_FORMAT_ERROR;
			goto exit;
		}	

		ret = getPeerMsg(cid, start_msgid, count, msgs, lastmsgid);
		
		if(ret < 0){
			PeerErrorLog(getPackSN(), "get_peer_message", cid, DB_ERROR);
			ret = ret;
			goto exit;
		}

		//construct resp
		gresp->set_last_msgid(lastmsgid);

		for(size_t i = 0; i < msgs.size(); ++i){
			Message* pm = gresp->add_msgs();	
			*pm = msgs[i];
			os << "<action:get_peer_message> <to:"
				<< cid << "> <from:" 
				<< msgs[i].from() << "> <msgid:"
				<< msgs[i].id() << "> <time:"
				<< msgs[i].time() << "> <msg_sn:"
				<< msgs[i].sn() << ">\n";
		}

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
		ret = 0;
exit:
		std::string payload;
		resppk.SerializeToString(&payload);

		ret = sendServiceResponse(ret, payload);	
		
		return ret;
	}

	int PeerCon::handleRecvPeerMessage(const PeerPacket& reqpk){
		int ret = 0;
		TimeRecorder t("PeerCon::handleRecvMessage");

		if(!reqpk.has_recv_peer_msg_resp()){
			FormatErrorLog(getPackSN(), "recv_peer_message");
			return INPUT_FORMAT_ERROR;	
		}	

		const RecvPeerMessageResponse& gmsgreq = reqpk.recv_peer_msg_resp();
		//get message from db
		const Message& msg = gmsgreq.msg();

		MsgDB* c = DBConn::getMsgDB();
		if(!c){
			PeerErrorLog(getPackSN(), "recv_peer_message", msg.to(), DB_ERROR);
			return DB_ERROR;
		}

		c->delMsgs("peer_" + msg.to(), msg.id() - MSG_LEFT_COUNT, -1);

		ALogTrace("PeerServer")
			<< "<sn:" << getPackSN()
			<< "> <action:recv_peer_message> <to:" 
			<< msg.to() << "> <msgid:" 
			<< msg.id() << "> <from:"
			<< msg.from() << "> <time:"
			<< msg.time() << "> <msg_sn:" 
			<< getPackSN() << "> <errstr:" 
			<< getErrStr(ret) << ">";
		return ret;
	}


	int PeerCon::handleSendPeerMessage(const PeerPacket& reqpk){

		int ret = 0;

		TimeRecorder trcd("PeerCon::handleSendMessage");

		int64 msgid = 0;
		PeerPacket resppk;

		//construct resp
		resppk.set_cmd(SEND_PEER_MESSAGE_RESP);
		SendPeerMessageResponse* gresp = resppk.mutable_send_peer_msg_resp();	
		Message* resppm = gresp->mutable_msg();

		const SendPeerMessageRequest& smsgreq = reqpk.send_peer_msg_req();
		const Message&  pm = smsgreq.msg();

		string payload;
		string resppayload;

		Settings *pSettings = Singleton<Settings>::instance();			
		//send to reciver 
		PeerPacket msgpk;
		msgpk = reqpk;
		SendPeerMessageRequest* spreq = msgpk.mutable_send_peer_msg_req();
		Message* pmsg = spreq->mutable_msg();
		*pmsg = *resppm;

		if(!reqpk.has_send_peer_msg_req()){
			FormatErrorLog(getPackSN(), "send_peer_message");
			ret = INPUT_FORMAT_ERROR;
			goto exit;
		}

		*(resppm) = pm;
		
		resppm->set_sn(getPackSN());
		resppm->set_time(gettime_ms());

		ret = sendPeerMsg(*resppm, msgid);
		if(ret < 0){
			PeerErrorLog(getPackSN(), "send_peer_message", pm.to(), DB_ERROR);
			goto exit;
		}		

		resppm->set_id(msgid);

	exit:
		if(ret >= 0){
			msgpk.SerializeToString(&payload);
			sendServiceRequestToClient(pm.to(), getPackSN(), payload);
			resppk.SerializeToString(&resppayload);	
		}


		if(pSettings->RespToMutiDevice){
			//when muti device online at the same time, 
			//should send the response
			//to all the device			
			sendServiceResponseToClient(pm.from(),
				getPackSN(), ret, resppayload);
		}else{
			sendServiceResponse(ret, resppayload);
		}

		//construct resp
		PLogTrace("PeerServer")
			<< "<action:send_peer_message> <to:" << pm.to()
			<< "> <from:" << pm.from() 
			<< "> <data:" << base64Encode(pm.data())
			<< "> <msg_sn:" << getPackSN() << "> <status:" 
			<< ret << "> <msgid:" << msgid << "> <errstr:"
			<< getErrStr(ret) << ">";
		return ret;	
	}

	PeerConFac	g_peerconfac;

	SvConFactory* getPeerConFactory(){
		return &g_peerconfac;
	}

	int PeerCon::handleServiceRequest(const std::string& payload){
		int ret = 0;
		PeerPacket reqpack;

		if(!reqpack.ParseFromString(payload)){
			FormatErrorLog(getPackSN(), "handle_peer_cmd");
			return INPUT_FORMAT_ERROR;
		}

		switch(reqpack.cmd()){
			case	SEND_PEER_MESSAGE_REQ:
				ret = handleSendPeerMessage(reqpack);
				break;
			case	GET_PEER_MESSAGE_REQ:
				ret = handleGetPeerMessage(reqpack);
				break;
			default:
				FormatErrorLog(getPackSN(), "handle_peer_cmd");
				break;
		}

		return ret;
	}

	int PeerCon::handleServiceResponse(int svtype, int status, 
		const std::string& payload, const std::string& callback){

		int ret = 0;
		
		if(status < 0){
			ALogError("PeerServer") 
				<< "<sn:" << getPackSN()
				<< "> <action:handleServiceResponse> <status:"
				   "0> <errstr:STATUS_OK> <svtype:" << svtype
				<< "> <resp_status:" << status;
			return 0;
		}

		if(svtype != SERVICE_TYPE_PEER){
			ALogError("PeerServer") 
				<< "<sn:" << getPackSN()
				<< "> <action:handleServiceResponse> <status:"
				<< (int)SERVICE_TYPE_ERROR 
				<< "> <errstr:SERVICE_TYPE_ERROR>";
			return 0;
		}
		PeerPacket reqpk;
		if(!reqpk.ParseFromArray(payload.data(), 
			payload.size())){
			ALogError("PeerServer")
				<< "<sn:" << getPackSN()
				<< "<action:handleServiceResponse> <status:"
				<< (int)(INPUT_FORMAT_ERROR)
				<< "> <errstr:INPUT_FORMAT_ERROR>";

		}
		ret = handleRecvPeerMessage(reqpk);
		return 0;
	}	
	
}
