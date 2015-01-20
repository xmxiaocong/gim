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

	int32 constructServiceRequest(const std::string& sessid,
		const std::string& tosessid,
		int32 service_type,
		const std::string& sn,
		const std::string& payload,
		const std::string& callback,
		std::string& req);

	int32 constructServiceResponse(const std::string& sessid,
		const std::string& tosessid,
		int32 status,
		int32 service_type,
		const std::string& sn,
		const std::string& payload,
		const std::string& callback,
		std::string& req);
	

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
