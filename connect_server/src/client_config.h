#ifndef __CLI_CONFIG_H__
#define __CLI_CONFIG_H__


namespace gim{

class CliConfig{
public:
	int Enc;
	int AliveMs;
	int MinType;
	int MaxType;
	int ListenPort;
	int MaxReqQueSize;
	int MaxPackCntPerMin;
	int StartThreadIdx;
	int ThreadCnt;
};

}

#endif
