#include "ef_timer.h"
#include "ef_connection.h"

namespace	ef{

	Timer::Timer(time_tv timeouttime)
		:m_timeouttime(timeouttime)
	{

	}

	Timer::~Timer(){

	}

	ConnectionTimer::ConnectionTimer(Connection* con, 
		int32 id, int32 timeout_ms)
		:m_con(con), m_id(id), m_status(STATUS_INIT){
		timeval	tv;
		gettimeofday(&tv, NULL);
		tv.tv_sec += timeout_ms / 1000;
		tv.tv_usec += timeout_ms % 1000 * 1000;	
		setTimeoutTime(tv);
	}

	int32 ConnectionTimer::timeout(EventLoop* l){
		m_status = STATUS_STOP;
		return m_con->handleTimer(l, m_id);
	}	

};

