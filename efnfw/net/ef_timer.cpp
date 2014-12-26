#include "ef_timer.h"
#include "ef_connection.h"

namespace	ef{

	Timer::Timer(Connection *con, 
		int32 id,
		time_tv timeouttime, 
		TimerHandler *handler)
		:m_con(con), 
		m_handler(handler),
		m_id(id), 
		m_timeouttime(timeouttime)
	{

	}

	Timer::~Timer(){

	}

	int32	Timer::timeout(EventLoop* l){
		int32 ret = 0;
		if(m_con){
			//del timer at first,because connection may recycle 
			//when handleTimer
			Timer t;
			m_con->findDelTimer(m_id, t);
			ret = m_con->handleTimer(l, m_id);
			if(ret < 0)
				m_con->onFail();
			return ret;
		}
		ret = m_handler->handleTimer(l, m_id);
		return ret;
	}

	int32 tv_cmp(struct timeval t1, struct timeval t2){
		if(t1.tv_sec < t2.tv_sec){
			return	-1;
		}else	if(t1.tv_sec > t2.tv_sec){
			return	1;
		}else{
			if(t1.tv_usec < t2.tv_usec){
				return	-1;
			}else	if(t1.tv_usec > t2.tv_usec){
				return	1;
			}else{
				return	0;
			}
		}
	}

	struct timeval tv_diff(struct timeval t1, struct timeval t2){
		struct	timeval	ret;

		ret.tv_sec = t1.tv_sec - t2.tv_sec;

		if(t1.tv_usec < t2.tv_usec){
			ret.tv_sec -= 1;
			t1.tv_usec += 1000000;
		}

		ret.tv_usec = t1.tv_usec - t2.tv_usec;

		return	ret;
	}


};

