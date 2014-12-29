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
	Json::Value cgcfg, mbcfg;
	cgcfg["UrlList"].append("127.0.0.1:7381");
	RedisCG cg(cgcfg, print);
	RdsMbFactory rdsfac;
	mbcfg["ExpiryTime"] = 200;
	mbcfg["Capability"] = 10;
	RedisMb *mb = (RedisMb *)rdsfac.createNewMbAdpt(mbcfg);
	mb->bindCG(&cg);
	string key("key");

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
	
