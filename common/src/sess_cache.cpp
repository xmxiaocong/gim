#include "sess_cache.h"
#include "def_sess_cache.h"

namespace gim{

SsChFactory* SsChFactory::g_fct = NULL;

int SsChFactory::init(const Json::Value& conf){
	g_fct = create(conf);
	return 0;
}

void SsChFactory::free(){
	delete g_fct;
	g_fct = NULL;
}

SsChFactory* SsChFactory::get(){
	return g_fct;
}

SsChFactory* SsChFactory::create(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefSessCache"){
		SsChFactory* c = new DefSsChFactory(conf);
		return c;
	}

	return NULL;
}

}
