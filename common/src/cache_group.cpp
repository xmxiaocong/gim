#include "cache_group.h"
#include "def_cache_group.h"

namespace gim{

CacheGroup* createCacheGroup(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefCacheGroup"){
		DefCacheGroup* c = new DefCacheGroup();

		if(c->init(conf) >= 0){
			return c;
		}
		
		delete c;	
	}

	return NULL;
}


}
