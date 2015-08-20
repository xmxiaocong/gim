#include "cache_group.h"
#include "def_cache_group.h"

namespace gim{


CacheGroupFactory* CacheGroupFactory::g_fac = NULL;

int CacheGroupFactory::init(const Json::Value& config){
	g_fac = create(config);
	return 0;
}

void CacheGroupFactory::free(){
	delete g_fac;
	g_fac = NULL;
}

CacheGroupFactory* CacheGroupFactory::get(){
	return g_fac;
}

CacheGroupFactory* CacheGroupFactory::create(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefCacheGroup"){
		CacheGroupFactory* c = new DefCacheGroupFactory(conf);

		return c;
	}

	return NULL;
}


}
