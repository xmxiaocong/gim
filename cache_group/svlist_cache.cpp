#include "svlist_cache.h"

namespace gim{

int DefSvLstCache::getEnableList(int type, vector<Serv> &servlist){
	return 0;
}

int DefSvLstCache::getDisableSvIDList(int type, vector<int> &servlist){
	return 0;
}

int DefSvLstCache::getAllList(int type, vector<Serv> &servlist){
	return 0;
}

int DefSvLstCache::addServ(int type, const Serv &serv){
	return 0;
}

int DefSvLstCache::updateServ(int type, const Serv &serv){
	return 0;
}

int DefSvLstCache::deleteServ(int type, int id){
	return 0;
}

int DefSvLstCache::enableServ(int type, int id){
	return 0;
}

int DefSvLstCache::disableServ(int type, int id){
	return 0;
}

SvLstCache* SvLstChFactory::getSvLstCache(const Json::Value& config){
	return NULL;
} 

}

