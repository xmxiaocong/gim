#include <iostream>
#include <fstream>
#include "svlist_cache.h"

using namespace std;
using namespace gim;

int printServerList(const vector<Serv>& svlst){
	for(size_t i = 0; i < svlst.size(); ++i){
		std::cout << "{id:" << svlst[i].id
			<< ", type:" << svlst[i].type
			<< ", config:" << svlst[i].v.toStyledString()
			<< "}, \n";
	}

	return 0;
}

int main(int argc, const char** argv){
	if(argc < 2){
		std::cout << "svlist_cache_test <config>\n";
		return -1;
	}
		

	const char* config = argv[1];
	SvLstChFactory sf;

	Json::Reader reader;
	Json::Value root;

	fstream f(config);
	
	if(!reader.parse(f, root, false)){
		std::cout << "parse config file fail!\n";
		return -1;
	}

	Json::Value& svconf = root["ServerList"];

	SvLstCache* c = sf.getSvLstCache(svconf);

	int ret = 0;

	vector<Serv> servlist;

	ret = c->getEnableList(0, servlist);

	printServerList(servlist);

	return 0;	
}
