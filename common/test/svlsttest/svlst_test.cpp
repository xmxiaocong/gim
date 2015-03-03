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

class TestSvLstListener:public SvLstListener{
public:
	virtual int onListChange(int type, vector<Serv> &servlist){
		printServerList(servlist);
		return 0;
	}
	virtual int onDisableListChange(int type, vector<int> &servlist){
		return 0;
	}
};


int main(int argc, const char** argv){
	if(argc < 2){
		std::cout << "svlist_cache_test <config>\n";
		return -1;
	}
		

	const char* config = argv[1];

	Json::Reader reader;
	Json::Value root;

	fstream f(config);
	
	if(!reader.parse(f, root, false)){
		std::cout << "parse config file fail!\n";
		return -1;
	}

	Json::Value& svconf = root["ServerList"];
	SvLstChFactory* sf = SvLstChFactory::create(svconf);

	SvLstCache* c = sf->newSvLstCache();

	int ret = 0;

	vector<Serv> servlist;
	TestSvLstListener l;

	c->setServerListListener(&l);

	ret = c->watchServerList(0);


	return 0;	
}
