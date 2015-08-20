#include "push_common.h"
#include "base/ef_utility.h"
//#include <iostream>

namespace gim{

	_JsonArgs::_JsonArgs(const std::string& k, ...)
		:key(k)
	{
		int j = 0;
		va_list arg_ptr;
		va_start(arg_ptr, k);
		int loopcount=0;
		while(loopcount++ < ARGMAX)
		{
			j = va_arg(arg_ptr, int);
			if ( j == ARGEND){
				break;
			}
			types.push_back(j);
		}
		va_end(arg_ptr);
	}

	bool pushRequestJsonCheck(const Json::Value& jargs){
		static std::vector<JsonArg> s_push_args;
		if(s_push_args.empty()){
			s_push_args.push_back(JsonArg("sn", Json::stringValue, Json::intValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("type", Json::intValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("from", Json::stringValue, Json::intValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("to", Json::stringValue, Json::intValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("data", Json::stringValue, Json::objectValue, Json::arrayValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("time", Json::stringValue, Json::intValue, JsonArg::ARGEND));
			s_push_args.push_back(JsonArg("expire", Json::stringValue, Json::intValue, JsonArg::ARGEND));
		}

		for (unsigned int n=0; n<s_push_args.size();++n){
			const JsonArg& arg =  s_push_args[n];
			if (!jargs.isMember(arg.key)){
				//std::cout << "no member:" << arg.key << std::endl;
				return false;
			}
			bool hit = false;
			for (unsigned int i=0; i<arg.types.size();++i){
				if(jargs[arg.key].type() == arg.types[i]){
					hit = true;
					break;
				}
			}
			if(!hit){
				//std::cout << "member:" << arg.key <<  " type=" << jargs[arg.key].type() << " error" << std::endl;
				return false;
			}
		}

		return true;
	}

	int64 strtoi64(const std::string& src){
		int64 t;
		std::stringstream  ss;
		ss << src;
		ss >> t;
		return t;
	}


	void messageJsonToProtobuf(const Json::Value& v, Message& msg){
		msg.set_type(v["type"].asInt());
		msg.set_from(v["from"].asString());

		std::stringstream ss;
		ss << v["time"].asString();
		ef::uint64 t;
		ss >> t;
		msg.set_time(t);
		
		ss.clear();
		ef::uint64 expire;
		ss << v["expire"].asString();
		ss >> expire;
		msg.set_expire(expire);

		msg.set_to(v["to"].asString());
		msg.set_sn(v["sn"].asString());

		if(v["data"].isString())
			msg.set_data(v["data"].asString());
		else
			msg.set_data(Json::FastWriter().write(v["data"]));

		//msg.set_id(0);
	}
	void messageProtobufToJson(const Message& gm, Json::Value& vm){
		vm["id"] = ef::itostr(gm.id());
		vm["to"] = gm.to();
		vm["time"] = ef::itostr(gm.time());
		vm["expire"] = ef::itostr(gm.expire());
		vm["from"] = gm.from();
		vm["type"] = gm.type();
		vm["data"] = gm.data();
		vm["sn"] = gm.sn();
	}
	
}