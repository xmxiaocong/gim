#include "redis_client.h"
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace gim;

void log2cout(const string &data)
{
	std::cout << data << std::endl << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		std::cout << "usage:redistest <servaddr> <servport> <passwd>" << std::endl;
		return -1;
	}
	RedisCli redcli;
	//redcli.setCmdLog(log2cout);
	if (redcli.connect(argv[1], atoi(argv[2]), argv[3]) < 0) {
		std::cout << "connect server fail" << std::endl;
		return -1;
	}
	string value;
	redcli.strSet("testkey", "testvalue");
	redcli.strGet("testkey", value);
	std::cout << value << std::endl;
	string oldvalue;
	redcli.strGetSet("testkey", "newvalue", oldvalue);
	value = "";
	redcli.strGet("testkey", value);
	std::cout << "newvalue:" << value << ",oldvalue:" << oldvalue << std::endl;
	
	int64 afterIncr;
	redcli.strIncr("key", afterIncr);
	std::cout << afterIncr << std::endl;
	redcli.strIncrBy("key", 100, afterIncr);
	std::cout << afterIncr << std::endl;

	redcli.keyDel("testkey");
	int exists;
	redcli.keyExists("key", exists);
	std::cout << exists << std::endl;
	redcli.keyExpire("key", 10);
	redcli.keyDel("key");
	redcli.keyExists("key", exists);
	std::cout << exists << std::endl;

	{
	redcli.hashSet("hashkey", "field0", "value0");
	redcli.hashSet("hashkey", "field1", "value1");
	string value;
	redcli.hashGet("hashkey", "field1", value);
	std::cout << value << std::endl;
	map<string, string> mfv, mfv1;
	mfv.insert(pair<string, string>("field2", "value2"));
	mfv.insert(pair<string, string>("field3", "value3"));
	redcli.hashMSet("hashkey", mfv);
	redcli.hashGetAll("hashkey", mfv1);
	map<string, string>::iterator it;
	for (it = mfv1.begin(); it!= mfv1.end(); it++) {
		std::cout << it->first << ":" << it->second << std::endl;
	}
	vector<string> fields;
	fields.push_back("field1");
	fields.push_back("field2");
	redcli.hashMDel("hashkey", fields);
	mfv1.clear();
	redcli.hashGetAll("hashkey", mfv1);
	for (it = mfv1.begin(); it!= mfv1.end(); it++) {
		std::cout << it->first << ":" << it->second << std::endl;
	}
	redcli.hashSet("hashkey", "field4", "value4");
	int exists;
	redcli.hashExists("hashkey", "field4", exists);
	std::cout << exists << std::endl;
	redcli.hashDel("hashkey", "field4");
	redcli.hashExists("hashkey", "field4", exists);
	std::cout << exists << std::endl;	
	}
	

	return 0;
}
