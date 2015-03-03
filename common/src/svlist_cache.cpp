#include "svlist_cache.h"
#include "def_svlist_cache.h"

namespace gim{

SvLstChFactory* SvLstChFactory::g_fct = NULL;

int SvLstChFactory::init(const Json::Value& conf){
	g_fct = create(conf);
	return 0;
}

void SvLstChFactory::free(){
	delete g_fct;
	g_fct = NULL;
}

SvLstChFactory* SvLstChFactory::get(){
	return g_fct;
}

SvLstChFactory* SvLstChFactory::create(const Json::Value& config){

	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefSvLstCache"){
		DefSvLstChFactory* c = new DefSvLstChFactory(conf);
		return c;
	}

	return NULL;
} 

}

