#include "svlist_cache.h"
#include "def_svlist_cache.h"

namespace gim{

SvLstCache* SvLstChFactory::getSvLstCache(const Json::Value& config){

	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefaultType"){
		DefSvLstCache* c = new DefSvLstCache();

		if(c->init(conf) >= 0){
			return c;
		}
		
		delete c;	
	}

	return NULL;
} 

}

