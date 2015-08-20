#ifndef __DISPATCH_CFG_H__
#define __DISPATCH_CFG_H__

#include "json/json.h"
#include <string>

namespace gim{

class DPConfig{
public:
	int ID;
	int ThreadCnt;
	int Daemon;
	int ListenPort;
	int StartThreadIdx;
	std::string ListenIP;
	std::string zkurl;
	std::string ConsvPath;
	std::string LogConfig;
	std::string NetLogName;
};

};
#endif
