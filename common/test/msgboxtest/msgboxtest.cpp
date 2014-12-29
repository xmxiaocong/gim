#include <iostream>
#include "redis_mb.h"
#include "redis_cg.h"

using namespace std;
using namespace gim;

void print(const string &data)
{
	cout << data << endl;
}

int main(void)
{
	Json::Value cfg;

	Json::Value ipcfg;
	ipcfg["ipaddr"] = "127.0.0.1";
	ipcfg["port"] = 7381;
	cfg["RedisCGCfg"]["UrlList"].append(ipcfg);
	ipcfg["port"] = 7382;
	cfg["RedisCGCfg"]["UrlList"].append(ipcfg);
	ipcfg["port"] = 7383;
	cfg["RedisCGCfg"]["UrlList"].append(ipcfg);
	cfg["RedisMbCfg"]["ExpiryTime"] = 200;
	cfg["RedisMbCfg"]["Capability"] = 10;

	RedisCG cg(cfg["RedisCGCfg"]);
	cg.setCmdLog(print);
	
	RdsMbFactory rdsfac;
	RedisMb *mb = (RedisMb *)rdsfac.createNewMbAdpt(cfg["RedisMbCfg"]);
	mb->bindCG(&cg);
	string key("keysd");

	Message msg;
	msg.set_to("234242145");
	int64 newid;
	mb->incrId(key, newid);
	msg.set_id(newid);
	msg.set_from("8987945734");
	msg.set_type(0);
	msg.set_sn("dalijlid9da3w40j09u");
	msg.set_data("hello");
	mb->addMsg("mb_" + key, msg);
	vector<Message> vmsg;
	mb->getMsgs("mb_" + key, 0, 100, vmsg);
	vector<Message>::iterator it;
	for (it = vmsg.begin(); it != vmsg.end(); it++) {
		cout << it->id() << endl;
	}
	mb->clear("mb_" + key);
	delete mb;
	return 0;	
}
	
