#include "push_msg_db.h"
#include "base/ef_tsd_ptr.h"
#include <iostream>

namespace gim{

	ef::TSDPtr<RedisMQ> g_pushMQ;
	Json::Value PushMsgDBM::g_push_db_cfg;

	void PushMsgDBM::initConfig(const Json::Value& conf){
		g_push_db_cfg = conf;
	}

	RedisMQ* PushMsgDBM::MQ(){
		RedisMQ* r = g_pushMQ.get();
		if (!r){
			std::cout << "MQ config:/n" << g_push_db_cfg.toStyledString() << std::endl;
			r = new RedisMQ(g_push_db_cfg);
			g_pushMQ.set(r);
		}

		return r;
	}
}