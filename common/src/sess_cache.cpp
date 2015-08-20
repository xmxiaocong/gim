#include "sess_cache.h"
#include "zk_client.h"
#include "def_sess_cache.h"
#include "def_sess_cache_group.h"


namespace gim{
SsChFactory* SsChFactory::g_fact = NULL;

int SsChFactory::initSsChFactory(ZKClient* zkc, const Json::Value& conf){
	const Json::Value& typev = conf["Type"];
	if(!typev.isString()){
		return -1;
	}	
	
	const std::string& type = typev.asString();
	if(type == "DefSessCache"){
		int ret = 0;
		const Json::Value& pathv = conf["Path"];
		if (!pathv.isString()){
			return -3;
		}

		DefSsChFactory* f = new DefSsChFactory();
		ret = f->init(zkc, pathv.asString());
		if(0 != ret){
			delete f;
			return ret;
		}
		g_fact = f;
		return 0;
	}
	else if(type == "DefSessCacheGroup"){
		int ret = 0;
		const Json::Value& paths = conf["Paths"];
		if(!paths.isArray())
			return -3;

		std::vector<string> vec;
		for (unsigned int n = 0; n < paths.size();++n){
			vec.push_back(paths[n].asString());
		}
		DefSsChGrpFactory* f = new DefSsChGrpFactory();
		ret = f->init(zkc, vec);
		if(0 != ret){
			delete f;
			return ret;
		}
		g_fact = f;
		return 0;
	}

	return -2;
}

int SsChFactory::freeSsChFactory(){
	delete g_fact;
	return 0;
}

SsChFactory* SsChFactory::getSsChFactory(){
	return g_fact;
}

}
