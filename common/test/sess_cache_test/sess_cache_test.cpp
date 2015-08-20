#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "sess_cache.h"
#include "base/ef_utility.h"
#include "zk_client.h"
#include "base/ef_thread.h"
#include <json/json.h>
#include "log_init.h"
#include "net/ef_net_log.h"
#include "base/ef_loader.h"


using namespace std;
using namespace gim;

int printSessList(const vector<Sess>& sslst){
	for(size_t i = 0; i < sslst.size(); ++i){
		std::cout << "{" << sslst[i].id() << "}, ";
	}

	return 0;
}

void* test_job(void* ctx){
	while(1){
		sleep(1);
		int r =  rand();
		int cmd =r%3;
		string cid = "sess_cache_test_cid_007";
		string sessid = "sess_cache_test_cid_007";

		int ret = 0;
		if(cmd == 0){
			vector<Sess> vs;
			ret =   SsChFactory::getSsChFactory()->getSessCache()->getSession(cid, vs);
			std::cout << "get result:" << ret <<  ",size=" << vs.size() << std::endl;
			//printSessList(vs);
		}else if(cmd == 1){
			Sess s;
			s.set_id(cid);
			s.set_sessid(sessid);
			ret =  SsChFactory::getSsChFactory()->getSessCache()->setSession(s);
			std::cout << "set result:" << ret << std::endl;
		}else if(cmd == 2222){
			Sess s;
			s.set_id(cid);
			s.set_sessid(sessid);
			ret =  SsChFactory::getSsChFactory()->getSessCache()->delSession(s);
			std::cout << "del result:" << ret << std::endl;
		}
	}
	return NULL;
}
int main(int argc, const char** argv){

	std::cout << "exe <zkurl> <path> \n";

	std::string url = "127.0.0.1:12306";
	//std::string config = "{\"Type\":\"DefSessCache\", \"Path\":\"/sess_cache_config\"}";
	std::string config = "{\"Type\":\"DefSessCacheGroup\", \"Paths\":[\"/sess_cache_1\", \"/sess_cache_2\"]}";

	if(argc < 3){
		std::cout << "default zkurl:127.0.0.1:12306, {\"Type\":\"DefSessCache\", \"Path\":\"/sess_cache_config\"}" << std::endl;
	}
	else{
		url = argv[1];
		config = argv[2];
	}
	
	Json::Value vc;
	try{
		Json::Reader r;	
		if(!r.parse(config, vc)){
			std::cout << "parse json:" << config << " fail" <<std::endl;
			return -1;
		}
	}catch(const std::exception& e){
		std::cout << e.what() << std::endl;
		return -1;
	}
	gim::logInit("log.conf");
	ef::setNetLogName("sess_cache_log");

	 ZKClient zkc;
	 zkc.init(url);
	 
	 SsChFactory::initSsChFactory(&zkc, vc);

	 const int thread_count = 1;
	 ef::THREADHANDLE* pth = new ef::THREADHANDLE[thread_count];
	 for (int n=0;n<thread_count;n++){
		 ef::threadCreate(&pth[n], NULL, test_job, NULL);
	 }
	 
	 for (int n=0;n<thread_count;n++){
		 ef::threadJoin(&pth[n]);
	 }

	return 0;	
}
