#include "def_svlist_cache.h"
#include <stdlib.h>

namespace gim{

int DefSvLstCache::init(const Json::Value& config){

	int ret = 0;
	Json::Value::const_iterator itr = config.begin();
	
	for(; itr != config.end(); ++itr){
		ret = loadOneType(*itr);

		if(ret < 0)
			break;
	}


	return ret;
}


int DefSvLstCache::loadOneType(const Json::Value& config){

	int ret = 0;
	int t = 0;
	const Json::Value& tp = config["Type"];

	if(tp.isIntegral()){
		t = tp.asInt();
	}else if(tp.isString()){
		t = atoi(tp.asCString());
	}else{
		return -100;
	}

	const Json::Value& svlst = config["ServerList"];

	if(!svlst.isArray()){
		return -101;
	}

	Json::Value::const_iterator itr = svlst.begin();

	for(; itr != svlst.end(); ++itr){
		ret = loadOneServer(t, *itr);

		if(ret < 0)
			break;
	}

	SvLstListener* ln = getServerListListener();

	if(ret >= 0 && ln){
		ln->onListChange(t, m_svlsts[t]);
	}

	return ret;
	
}

int DefSvLstCache::loadOneServer(int type, const Json::Value& config){
	
	int id = 0;

	const Json::Value& d = config["ID"];

	if(d.isIntegral()){
		id = d.asInt();
	}else if(d.isString()){
		id = atoi(d.asCString());
	}else{
		return -100;
	}

	Serv s;
	
	s.type = type;
	s.id = id;
	s.v = config;
	
	m_svlsts[type].push_back(s);
	
	return 0;
}


int DefSvLstCache::addServer(int type, const Serv &serv){
	return 0;
}

int DefSvLstCache::updateServer(int type, const Serv &serv){
	return 0;
}

int DefSvLstCache::deleteServer(int type, int id){
	return 0;
}

int DefSvLstCache::enableServer(int type, int id){
	return 0;
}

int DefSvLstCache::disableServer(int type, int id){
	return 0;
}

}
