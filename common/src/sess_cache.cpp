#include "sess_cache.h"
#include "def_sess_cache.h"

namespace gim{

SessCache* SsChFactory::getSessCache(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefaultType"){
		DefSessCache* c = new DefSessCache();
		if(c->init(conf) >= 0){
			return c;
		}
		
		delete c;	
	}

	return NULL;
}

}
