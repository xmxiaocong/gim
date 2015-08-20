#ifndef LOGIC_COMMON_H
#define LOGIC_COMMON_H

#include <string>
#include "base/ef_btype.h"
#include "base/ef_log.h"
#include "base/ef_utility.h"

namespace ef{
	class EventLoop;
	class Connection;
};

namespace gim{
	class head;
	using namespace ef;

	int32 constructPacket(const head& h, 
		const std::string& body, std::string& pack);

	int32 decorationName(int32 svid, int32 conid,
		const std::string& n, std::string& dn);

	int32 getDecorationInfo(const std::string& dn, int32& svid,
		int32& conid, std::string& n);	

	int32 getSvTypeFromSessid(const std::string& ssid, int32& svtype);


	#define ALogTrace(a) logTrace(a) << "[" << __FUNCTION__\
		<< "] addr[" << getIp() << ":"\
		<< getPort() << "] <timestamp:" << gettime_ms() << "> " 
	#define ALogError(a) logError(a) << "[" << __FUNCTION__\
		<< "] addr[" << getIp() << ":"\
		<< getPort() << "] <timestamp:" << gettime_ms() << "> " 

	#define PLogError(a) ALogError(a) << "<sn:" << getPackSN() << "> "
	#define PLogTrace(a) ALogTrace(a) << "<sn:" << getPackSN() << "> "

};

#endif/*LOGIC_COMMON_H*/
