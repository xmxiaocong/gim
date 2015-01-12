#include "msg_db.h"
#include "redis_msg_db.h"

namespace gim{

MsgDB* MsgDBFactory::newMsgDB(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "RedisMI"){
		RedisMI* c = new RedisMI(conf);
		return c;
	}

	return NULL;
}

}
