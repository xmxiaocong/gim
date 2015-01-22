#ifndef LOGIC_COMMON_H_
#define LOGIC_COMMON_H_

#include <string>
#include "common/ef_btype.h"
#include "common/ef_utility.h"

namespace gim
{
	class head;
	using namespace ef;
	enum
	{
		MAGIC_NUMBER = 0x20140417,
	};

	int32 constructRespPacket(int32 srvcmd, const std::string& body, std::string& respbuf);

	int32 constructReqPacket(const head& h, const std::string& body, std::string& respbuf);

	int32 constructServiceRequest(const std::string& sessid, int32 service_type,
		const std::string& sn, const std::string& payload, std::string& req);

	int32 constructServiceResponse(const std::string& fromsession, const std::string& tosession, int32 status, int32 service_type,
		const std::string& sn, const std::string& payload, std::string& req);

};

#endif/*LOGIC_COMMON_H*/
