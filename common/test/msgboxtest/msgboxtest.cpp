#include <iostream>
#include <fstream>
#include "redis_mb.h"
#include "redis_cg.h"

using namespace std;
using namespace gim;

void print(const string &data)
{
	cout << data << endl;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cout << "usage:" << argv[0] << " <cfgfile>" << std::endl;
		return -1;
	}
	
	Json::Value cfg;
	Json::Reader reader;

	ifstream is(argv[1]);
	string strcfg;
	while (is) {
		string tmp;
		getline(is, tmp);
		strcfg += tmp;
	}
	
	if (!reader.parse(strcfg, cfg)) {
		std::cout << "parse config file fail" << std::endl;
		return -1;
	}

	RedisCG cg(cfg["RedisCGCfg"]);
	cg.setCmdLog(print);
	
	RdsMbFactory rdsfac;
	RedisMb *mb = (RedisMb *)rdsfac.createNewMbAdpt(cfg["RedisMbCfg"]);
	mb->bindCG(&cg);

	Message msg;
	msg.set_to("234242145");
	msg.set_id(420809);
	msg.set_from("8987945734");
	msg.set_type(0);
	msg.set_sn("dalijlid9da3w40j09u");
	msg.set_data("hello");
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	mb->addMsg("msgbox_344", msg);
	vector<Message> vmsg;
	int ret = mb->getMsgs("msgbox_344", 0, 100, vmsg);
	vector<Message>::iterator it;
	for (it = vmsg.begin(); it != vmsg.end(); it++) {
		cout << it->sn() << endl;
	}
//	mb->clear("msgbox_344");
	delete mb;
	return 0;	
}
	
