#ifndef __EF_TIMER_H__
#define __EF_TIMER_H__

#include "ef_common.h"
#include "ef_sock.h"

namespace ef{

	class Connection;
	class EventLoop;

	class TimerHandler{
	public:
		int32 handleTimer(EventLoop* l,int32 timer_id){
			return 0;
		}
	};

	struct time_tv{
	time_tv(long sec = 0,long usec = 0)
		:m_sec(sec), m_usec(usec){

		}

	time_tv(struct timeval tv)
		:m_sec(tv.tv_sec), m_usec(tv.tv_usec){
		}

	operator struct timeval(){
		timeval tv;
		tv.tv_sec = m_sec;
		tv.tv_usec = m_usec;
		return tv;
	}

	long m_sec;
	long m_usec;

	};

	class Timer{
	public:
		Timer(Connection *con = NULL, int32 id = 0, 
				time_tv timouttime = time_tv(), TimerHandler* handler = NULL);

		virtual ~Timer();

		virtual int32 timeout(EventLoop* l);

		time_tv getTimeoutTime(){
			return m_timeouttime;
		}

		Connection* getConnection(){
			return m_con;
		}

		int32 getId(){
			return m_id;
		}

	private:
		Connection *m_con;
		TimerHandler *m_handler;
		int32  m_id;
		time_tv  m_timeouttime;
	};



};

#endif/*EF_TIMER_H*/

