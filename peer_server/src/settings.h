#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "base/ef_singleton.h"
#include "base/ef_btype.h"
#include "json/json.h"
#include <string>

namespace gim{

using namespace ef;

class Settings
{
public:
	Settings();
	bool init();
	bool load(const char *filename);
	void print();
public:
	int ThreadCount;

	int Daemon;
	int Id;
	int ReconnectSpan;
	int KeepAliveSpan;
	int MsgBoxSize;
	int MsgExpireTime;
	int RespToMutiDevice;
	int ServiceType;

	int PushListenPort;

	std::string NLogLevel;
	std::string NLogPath;
	std::string LogConfig;

	Json::Value SvLstCacheConfig;
	Json::Value SessCacheConfig;	
	Json::Value MsgDBConfig;
};

}
#endif

