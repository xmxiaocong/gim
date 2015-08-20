#include <iostream>
#include "zk_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <json/json.h>
#include "zk_server_ob.h"
#include <algorithm>
#include "zk_config.h"
#include "zk_server_node.h"


using namespace gim;

int obCallBack(void* ctx, const children_map& children){
	std::cout << "obcallback\n";
	try{
		Json::FastWriter w;
		for (children_map::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			std::cout << it->first << ", json:" << w.write(it->second) << std::endl;
		}
	}
	catch (const std::exception &ex){
		std::cout <<  ex.what() << "\n";
	}

	return 0;
}

void cfgCallback(void* ctx, int version, const Json::Value& notify){
	std::cout << "cfgCallback\n";
	try{
		Json::FastWriter w;
		std::cout << w.write(notify) << std::endl;
	}
	catch (const std::exception &ex){
		std::cout <<  ex.what() << "\n";
	}
}

void logfn(void* context, const std::string& l){
	std::cout << l << std::endl;
}
int main(int argc, const char** argv)
{
	std::string url = "127.0.0.1:12306,127.0.0.1:22306,127.0.0.1:32306,127.0.0.1:42306,127.0.0.1:52306,127.0.0.1:62306";
	std::string path = "/dps";
	ZKClient c;
	c.setLogFn(NULL, logfn);
	c.init(url);

 	ZKServerListOb ob(&c, "/dps/ob");
 	ob.init(true, obCallBack, NULL);



	ZKConfig cfg(&c, "/dps/cfg", "");
	cfg.init(cfgCallback, NULL);

 	ZKServerNode node(&c, "/dps/node", "1");
 	node.init("ohe   \nhe");

	int count = 5;
	while (count > 0)
	{
#ifndef _WIN32
		sleep(1);
#endif // !_WIN32
	}

	return 0;
}
