#include "push_server_conn.h"
#include "push_common.h"
#include "push_msg_db.h"
#include "msg_head.h"

namespace gim{
	int PushSrvConn::handleServiceRequest(const ServiceRequest& req){
		int ret = 0;
		try{
			Json::Value jargs;
			Json::Reader r;
			if(!r.parse(req.payload(), jargs)){
				ret = PS_RESULT_JSON_ERROR;
				goto parse_end;
			}
			
			if(!jargs.isMember("cmd")){
				ret = PS_RESULT_ARGS_ERROR;
				goto parse_end;
			}

			int cmd = jargs["cmd"].asInt();
			switch (cmd){
				case PS_CMD_GET_PUSH_MSG:
					ret = handleGetMsgs(req, jargs);
					break;
				case PS_CMD_PEER_REQUEST:
					ret = handlePeer(req, jargs);
					break;
				default:
					ret = PS_RESULT_UNDEFINED_CMD;
					break;
			}
		}catch (const std::exception& e){
			ret = PS_RESULT_JSON_ERROR;
			PSLogError("PushServer") << "<action: PushSrvConn::handleServiceRequest> <conid:" 
				<< getId() << "> <result: json exception: " << e.what() << ">";
			goto parse_end;
		}

parse_end:
		if(ret < 0){
			PSLogError("PushServer") << "<action: PushSrvConn::handleServiceRequest> <conid:" 
				<< getId() << "> <result: " << ret  << ">";
			ServiceResponse response;
			response.set_sn(req.sn());
			response.set_to_type(-1);
			response.set_status(0);
			response.set_to_sessid(req.from_sessid());

			Json::Value vresp;
			vresp["cmd"] = PS_CMD_GET_PUSH_MSG_RESPONSE;
			vresp["result"] = ret;

			response.set_payload(Json::FastWriter().write(vresp));
			sendServiceResponse(response);
		}
		return 0;
	}

	int PushSrvConn::handleServiceResponse(const ServiceResponse& resp){
		PSLogTrace("PushServer") << "<action:handle server push response> <conid:" << getId() << ">";

		if(resp.status() >= 0){
			try{
				Json::Value v;
				Json::Reader r;
				if(r.parse(resp.payload(), v) && v.isMember("id") && v.isMember("uid")){
						string uid = v["uid"].asString();
						int64 msgid = strtoi64(v["id"].asString());
						PushMsgDBM::MQ()->del(uid, MSG_KEY(uid), msgid, msgid);
				}
			}catch(const std::exception& e){
				PSLogError("PushServer") << "<action:PushSrvConn::handleServiceResponse> <conid:" 
					<< getId() << "> <result: json exception:" <<  e.what() << ">";
			}
		}

		return 0;
	}

	int PushSrvConn::handleGetMsgs(const ServiceRequest& req, const Json::Value& jargs){
		PSLogTrace("PushServer") << "<action:handleGetMsgs> <conid:" << getId() << ">";

		static vector<JsonArg> s_getmsg_args;
		if(s_getmsg_args.empty()){
			s_getmsg_args.push_back(JsonArg("start_id", Json::intValue, JsonArg::ARGEND));
			s_getmsg_args.push_back(JsonArg("count", Json::intValue, JsonArg::ARGEND));
			s_getmsg_args.push_back(JsonArg("uid", Json::stringValue, JsonArg::ARGEND));
		}

		for (unsigned int n=0; n<s_getmsg_args.size();++n){
			const JsonArg& arg =  s_getmsg_args[n];
			if (!jargs.isMember(arg.key)){
				return PS_RESULT_REDIS_ERROR;
			}
			bool hit = false;
			for (unsigned int i=0; i<arg.types.size();++i){
				if(jargs[arg.key].type() == arg.types[i]){
					hit = true;
					break;
				}
			}
			if(!hit)
				return PS_RESULT_JSON_ERROR;
		}

		int startid = jargs["start_id"].asInt();
		int count = jargs["count"].asInt();
		const int get_msg_max_len = 100;
		if(count > get_msg_max_len)
			count = get_msg_max_len;
		std::string to = jargs["uid"].asString();
		std::string mkey = MSG_KEY(to);

		vector<Message> vm;
		int ret = PushMsgDBM::MQ()->get(to, mkey, startid, count, vm);
		if(ret < 0){
			PSLogError("PushServer") << "<action:handleGetMsgs> <conid:" 
				<< getId() << "> <result: get error>";
			return PS_RESULT_REDIS_ERROR;
		}
		
		Json::Value jresp;
		for (unsigned int n=0;n<vm.size();n++){
			const Message& m = vm[n];
			Json::Value vtemp;
			messageProtobufToJson(m, vtemp);
			jresp["msgs"].append(vtemp);
		}

		int64 maxid=0;
		ret = PushMsgDBM::MQ()->getId(to, LAST_MSG_ID_KEY(to), maxid);
		if (ret < 0){
			return PS_RESULT_REDIS_ERROR;
		}
		jresp["maxid"] = itostr(maxid);

		if(startid > 0)
			PushMsgDBM::MQ()->del(to, mkey, 0, startid-1);
		
		ServiceResponse response;
		response.set_sn(req.sn());
		response.set_to_type(-1);
		response.set_status(0);
		response.set_to_sessid(req.from_sessid());
		jresp["cmd"] = PS_CMD_GET_PUSH_MSG_RESPONSE;
		jresp["result"] = ret;
		response.set_payload(Json::FastWriter().write(jresp));
		sendServiceResponse(response);

		return 0;
	}

	int PushSrvConn::handlePeer(const ServiceRequest& req, const Json::Value& jargs){
		PSLogTrace("PushServer") << "<action:handlePeer> <conid:" << getId() << ">";

		if(!pushRequestJsonCheck(jargs)){
			PSLogError("PushServer") << "<action:handlePeer> <conid:" 
				<< getId() << "> <status: check json members error>";
			return PS_RESULT_ARGS_ERROR;
		}

		Message msg;
		messageJsonToProtobuf(jargs,msg);

		const std::string& to = jargs["to"].asString();
		const std::string& sn = jargs["sn"].asString();

		std::string mkey = MSG_KEY(to);
		std::string idkey = LAST_MSG_ID_KEY(to);

		int64 msgid =-1;
		int ret = PushMsgDBM::MQ()->incrId(to, idkey, msgid);
		if(ret < 0){
			PSLogError("PushServer") << "<action:handlePeer> <conid:" 
				<< getId() << "> <status: incrId error:" << ret << ">";
			return PS_RESULT_INCR_MSGID_ERROR;
		}

		msg.set_id(msgid);
		ret  = sendJ2C(msg);

		if(msg.expire() > 0){
			if(PushMsgDBM::MQ()->add(to, mkey, msg) < 0){
				PSLogError("PushServer") << "<action:handlePeer> <conid:" 
					<< getId() << "> <status: add redis error:>";
			}
		}

		ServiceResponse response;
		response.set_sn(req.sn());
		response.set_to_type(-1);
		response.set_status(0);
		response.set_to_sessid(req.from_sessid());

		Json::Value jresp;
		jresp["cmd"] = PS_CMD_PEER_RESPONSE;
		jresp["result"] = 0;
		jresp["msgid"] = ef::itostr(msgid);
		jresp["sn"] = sn;

		response.set_payload(Json::FastWriter().write(jresp));
		sendServiceResponse(response);
		return ret;
	}


	int PushSrvConn::sendJ2C(const Message& m){
		Json::Value v;
		v["cmd"] = PS_CMD_PUSH_REQUEST;
		messageProtobufToJson(m, v);

		ServiceRequest sreq;
		sreq.set_payload(Json::FastWriter().write(v));
		sreq.set_to_type(-1);
		sreq.set_sn(m.sn());

		int ret = sendToClient(m.to(), sreq);
		PSLogTrace("PushServer") << "<action:handle push send2c> <conid:" << getId() << "><to:" << m.to() << "><result=" << ret << ">";
		return ret == -20001?0:ret;
	}
}