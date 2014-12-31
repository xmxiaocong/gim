#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "sess_cache.h"

using namespace std;
using namespace gim;

int printSessList(const vector<Sess>& sslst){
	for(size_t i = 0; i < sslst.size(); ++i){
		std::cout << "{" << sslst[i].DebugString() << "}, ";
	}

	return 0;
}

int main(int argc, const char** argv){
	if(argc < 4){
		std::cout << "sess_test <config> <get/set/del> <cid> [sessid] []\n";
		return -1;
	}
		

	const char* config = argv[1];
	SsChFactory sf;

	Json::Reader reader;
	Json::Value root;

	fstream f(config);
	
	if(!reader.parse(f, root, false)){
		std::cout << "parse config file fail!\n";
		return -1;
	}

	Json::Value& svconf = root["SessionCache"];

	SessCache* c = sf.getSessCache(svconf);

	string cmd = argv[2];	
	string cid = argv[3];

	int ret = 0;
	if(cmd == "get"){
		vector<Sess> vs;
		ret = c->getSession(cid, vs);
		std::cout << "get result:" << ret << std::endl;
		printSessList(vs);
	}else if(cmd == "set"){
		Sess s;
		string sessid = argv[4];	
		s.set_cid(cid);
		s.set_sessid(sessid);
		ret = c->setSession(s);
		std::cout << "set result:" << ret << std::endl;
	}else if(cmd == "del"){
		Sess s;
		string sessid = argv[4];	
		s.set_cid(cid);
		s.set_sessid(sessid);
		ret = c->delSession(s);
		std::cout << "del result:" << ret << std::endl;
	}

	return 0;	
}
