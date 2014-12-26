#include "redis_client.h"
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace gim;

void log2cout(const string &data)
{
	std::cout << data << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		std::cout << "usage:redistest <servaddr> <servport> <passwd>" << std::endl;
		return -1;
	}
	RedisCli redcli;
	redcli.setCmdLog(log2cout);
	if (redcli.connect(argv[1], atoi(argv[2]), argv[3]) < 0) {
		std::cout << "connect server fail" << std::endl;
		return -1;
	}
	int64 ret;
	int64 afterincr;
	if ((ret=redcli.strIncrBy("key8", 1000, afterincr)) < 0) {
		std::cout << "execmd fail:" << ret << std::endl;
		return -1;
	}
	std::cout << afterincr << std::endl;

	return 0;
}
