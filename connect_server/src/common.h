#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include "base/ef_btype.h"
#include "base/ef_log.h"
#include "base/ef_utility.h"

namespace ef{
	class EventLoop;
	class Connection;
};

namespace gim{

using namespace ef;

	class head;

	int32 checkLen(ef::Connection& c); 

	int32 constructPacket(const head& h, 
		const std::string& body, std::string& pack);
	
	enum{
		EVENT_LOOP_PART_CNT = 4,
		EVENT_LOOP_ID_MASK = 1000000,
		FRE_CHECK_SPAN = 60,
	};

	int32 decorationName(int32 svid, int32 conid, 
		const std::string& n, std::string& dn);

	int32 getDecorationInfo(const std::string& dn, int32& svid,
		int32& conid, std::string& n);
	

	inline int32 serverEventLoopCount(int32 maxevlp){
		int32 svevcnt = maxevlp >= EVENT_LOOP_PART_CNT ?
			maxevlp / EVENT_LOOP_PART_CNT : 1;
		return svevcnt;
	}

	inline int32 getConnectionId(int32 evlpid, int32 conid){
		return (evlpid * EVENT_LOOP_ID_MASK)
			 + (conid % EVENT_LOOP_ID_MASK);
	}

	inline int32 getEventLoopId(int32 conid){
		return conid / EVENT_LOOP_ID_MASK;
	}




};

#endif/*COMMON_H*/
