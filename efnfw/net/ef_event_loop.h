#ifndef EF_NET_THREAD_H
#define EF_NET_THREAD_H

#include <map>
#include <set>
#include <list>
#include <string>
#include "ef_common.h"
#include "ef_sock.h"
#include "ef_timer.h"
#include "base/ef_thread.h"
#include "base/ef_atomic.h"
#include "base/ef_loop_buf.h"

namespace ef{

	class Device;
	class Connection;
	class Acceptor;
	class NetOperator;

	typedef int32 (*OBJ_CLEAN)(void* obj);

	class EventLoop{
	public:
		EventLoop();

		virtual ~EventLoop();

		int32 addNotify(Device *con, int32 noti);
		int32 clearNotify(Device *con);
		int32 modifyNotify(Device *con, int32 noti);

		int32 asynAddConnection(int32 id, Connection *con);
		int32 asynCloseConnection(int32 id);
		int32 asynCloseAllConnections();
		int32 asynSendMessage(int32 id, const std::string &msg);
		int32 asynAddTimer(Timer* t);
		int32 asynDelTimer(Timer* t);
		int32 addAsynOperator(NetOperator* op = NULL);

		int32 init();
		int32 run();
		int32 stop();

		int32 setObj(void* obj, OBJ_CLEAN cl){
			int32 ret = 0;
			if(m_clean)
				ret = m_clean(m_obj);
			m_obj = obj;
			m_clean = cl;
			return ret;
		}

		void* getObj(){
			return m_obj;
		}

		int32 getConId();

		int32 getId(){
			return m_id;
		}

		void setId(int32 id){
			m_id = id;
		}

		//the function below is not thread safe	
		int32 addTimer(Timer* tm);
		int32 delTimer(Timer* tm);
		int32 addConnection(int32 id, Connection *con);
		int32 delConnection(int32 id);
		int32 closeConnection(int32 id);
		Connection* getConnection(int32 id);
		int32 sendMessage(int32 id, const std::string &msg);
		int32 closeAllConnections();
		size_t ConnectionsCount();
	private:
		int32 startCtl();

		int32 delAllConnections();
		int32 delAllOp();

		int32 processTimers(time_tv &diff);
		int32 processOps();

		struct less
		{ // functor for operator<
			bool operator()
				(const Timer* _Left, const Timer* _Right) const
			{
				return *_Left < *_Right;
			}
		};


		enum{
			STATUS_INIT = 0,
			STATUS_RUNNING = 1,
			STATUS_CLOSING = 2,
			STATUS_CLOSED = 3,
		};

		int32 m_max_fds;
		volatile int32 m_status;
		SOCKET m_epl;
		SOCKET m_ctlfd;
		int32 m_ctlport;
		SOCKET m_ctlfd1;

		typedef std::map<int32, Connection*> con_map;
		con_map m_con_map;

		typedef std::set<Timer*, less> timer_map;
		timer_map m_timer_map;

		MUTEX m_opcs;
		volatile int32 m_ops_flag;

		LoopBuf m_ops;
		int32 m_cur_id;

		int32 m_id;
		void* m_obj;
		OBJ_CLEAN m_clean;	
	};

};

#endif/*EF_NET_THREAD_H*/


