#include "settings.h"
#include <fstream>
#include <iostream>
#include "base/ef_utility.h"
#include "net/ef_sock.h"


namespace gim{

Settings::Settings()
{
	init();
}

bool Settings::init()
{
	ClientListenPort = 3000;
	ServerListenPort = 1300;

	ThreadCount = 6;
	Daemon = false;
	
	return true;
}


bool Settings::load(const char *filename)
{
	Json::Reader reader;
	Json::Value root;
	std::fstream f(filename);

	if(!reader.parse(f, root, false)){
		std::cout << "parse config file fail!\n";
		return -1;
	}


	Daemon = root["Daemon"].asInt();
	Type = root["Type"].asInt();	
	Id = root["Id"].asInt();	

	ThreadCount = root["ThreadCount"].asInt();

	ef::getIPs(IPs);
	ef::getLocalIPs(LocalIPs); 
	ClientListenPort = root["ClientListenPort"].asInt();
	ServerListenPort = root["ServerListenPort"].asInt();
		
	StartTime = ef::gettime_ms();	

	CliMaxIdleMs = root["CliMaxIdleMs"].asInt();
	MaxCliCount = root["MaxCliCount"].asInt();
	MaxReqFrequency = root["MaxReqFrequency"].asInt();
	MaxAcceptSpeed = root["MaxAcceptSpeed"].asInt();
	MaxTimeDif = root["MaxTimeDif"].asInt();

	NLogLevel = root["NLogLevel"].asString();
	NLogPath = root["NLogPath"].asString();
	LogConfig = root["LogConfig"].asString();
	
	SvLstCacheConfig = root["SvLstCacheConfig"];
	SessCacheConfig = root["SessCacheConfig"];
	UserDBConfig = root["UserDBConfig"];

	return true;
}

static std::string strVectorStr(const std::vector<std::string> &v){
	std::string ret = "[";

	for(size_t i = 0; i < v.size(); ++i){
		ret += "\"";
		ret += v[i];
		ret += "\",";
	}

	ret += "]";

	return ret;
}

void Settings::print(){
	std::cout << "Daemon:" << Daemon << std::endl;
	std::cout << "Type:" << Type << std::endl;
	std::cout << "Id:" << Id << std::endl;

	std::cout << "LocalIPs:" << strVectorStr(LocalIPs) << std::endl;
	std::cout << "IPs:" << strVectorStr(IPs) << std::endl;
	std::cout << "ClientListenPort:" << ClientListenPort << std::endl;
	std::cout << "ServerListenPort:" << ServerListenPort << std::endl;
	std::cout << "CliMaxIdleMs:" << CliMaxIdleMs << std::endl;
	std::cout << "ThreadCount:" << ThreadCount << std::endl;
	std::cout << "LogConfig:" << LogConfig << std::endl;
	std::cout << "NLogLevel:" << NLogLevel << std::endl;
	std::cout << "NLogPath:" << NLogPath << std::endl;

	std::cout << "StartTime:" << StartTime << std::endl;

	std::cout << "MaxCliCount:" << MaxCliCount << std::endl;
	std::cout << "MaxReqFrequency:" << MaxReqFrequency << std::endl;
	std::cout << "MaxAcceptSpeed:" << MaxAcceptSpeed << std::endl;
	std::cout << "MaxTimeDif:" << MaxTimeDif << std::endl;

	std::cout << "SvLstCacheConfig:" << SvLstCacheConfig.toStyledString() 
		<< std::endl;

	std::cout << "SessCacheConfig:" << SessCacheConfig.toStyledString() 
		<< std::endl;
	
	std::cout << "UserDBConfig:" << UserDBConfig.toStyledString()
		<< std::endl;
}

}

