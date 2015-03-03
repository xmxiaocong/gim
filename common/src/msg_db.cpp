#include "msg_db.h"
#include "redis_msg_db.h"

namespace gim{

MsgDBFactory* MsgDBFactory::g_fct = NULL;

int MsgDBFactory::init(const Json::Value &conf){
	g_fct = create(conf);
	return 0;
}

void MsgDBFactory::free(){
	delete g_fct;
	g_fct = NULL;
}

MsgDBFactory* MsgDBFactory::get(){
	return g_fct;
}

MsgDBFactory* MsgDBFactory::create(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "RedisMI"){
		RedisMIFactory* c = new RedisMIFactory(conf);
		return c;
	}

	return NULL;
}

}
