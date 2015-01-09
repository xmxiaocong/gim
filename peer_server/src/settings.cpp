#include "settings.h"
#include <fstream>

namespace gim{

Settings::Settings()
{
	init();
}

bool Settings::init()
{

	ThreadCount = 6;

	Daemon = false;

	ReconnectSpan = 30000;
	
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
	ServiceType = root["ServiceType"].asInt();	
	Id = root["Id"].asInt();	

	ThreadCount = root["ThreadCount"].asInt();
		

	ReconnectSpan = root["ReconnectSpan"].asInt();
	KeepAliveSpan = root["KeepAliveSpan"].asInt();
	MsgBoxSize = root["MsgBoxSize"].asInt();
	MsgExpireTime = root["MsgExpireTime"].asInt();
	RespToMutiDevice = root["RespToMutiDevice"].asInt();

	PushListenPort = root["PushListenPort"].asInt();


	NLogLevel = root["NLogLevel"].asString();
	NLogPath = root["NLogPath"].asString();
	LogConfig = root["LogConfig"].asString();
	
	SvLstCacheConfig = root["SvLstCacheConfig"];
	SessCacheConfig = root["SessCacheConfig"];
	MsgDBConfig = root["MsgDBConfig"];

	return true;
}


void
Settings::print(){
	std::cout << "Daemon:" << Daemon << std::endl;
	std::cout << "Id:" << Id << std::endl;
	std::cout << "ServiceType:" << ServiceType << std::endl;
	std::cout << "ThreadCount:" << ThreadCount << std::endl;
	std::cout << "LogConfig:" << LogConfig << std::endl;
	std::cout << "NLogLevel:" << NLogLevel << std::endl;
	std::cout << "NLogPath:" << NLogPath << std::endl;
	std::cout << "ReconnectSpan:" << ReconnectSpan << std::endl;
	std::cout << "KeepAliveSpan:" << KeepAliveSpan << std::endl;
	std::cout << "MsgBoxSize:" << MsgBoxSize << std::endl;
	std::cout << "MsgExpireTime:" << MsgExpireTime << std::endl;
	std::cout << "RespToMutiDevice:" << RespToMutiDevice << std::endl;
	std::cout << "PushListenPort:" << PushListenPort << std::endl;

	std::cout << "SvLstCacheConfig:" << SvLstCacheConfig.toStyledString() 
		<< std::endl;

	std::cout << "SessCacheConfig:" << SessCacheConfig.toStyledString() 
		<< std::endl;
	std::cout << "MsgDBConfig:" << MsgDBConfig.toStyledString() 
		<< std::endl;
		
}

}

