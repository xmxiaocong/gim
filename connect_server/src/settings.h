#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <vector>
#include <string>
#include "json/json.h"
#include "base/ef_singleton.h"
#include "base/ef_btype.h"

namespace gim{


class Settings
{
public:
	Settings();
	bool init();
	bool load(const char *filename);
	void print();
public:
	int Daemon;
	int Type;
	int Id;

	int ThreadCount;

	std::vector<std::string> LocalIPs;
	std::vector<std::string> IPs;
	int ClientListenPort;
	int ServerListenPort;

	ef::int64 StartTime;

	int CliMaxIdleMs;
	int MaxCliCount;
	int MaxReqFrequency;
	int MaxAcceptSpeed;
	int MaxTimeDif;

	std::string NLogLevel;
	std::string NLogPath;
	std::string LogConfig;

	Json::Value SvLstCacheConfig;
	Json::Value SessCacheConfig;
	Json::Value UserDBConfig;
};

}
#endif

