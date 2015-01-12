#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_
#include "common/ef_sock.h"
#include "common/ef_thread.h"
#include "common/ef_loop_buf.h"
#include "opbase.h"
#include <string>
#include <vector>
#include <map>

using namespace ef;
namespace gim
{
	class EventLoop
	{
		friend class Op;
	public:
		EventLoop();
		~EventLoop();
		int32 startLoop();
		int32 asynAddOp(Op* op);
		int32 asynStop();
		CliConn* findConn(const std::string& cid);
		CliConn* addConn(const std::string& cid);
		int32 delConn(const std::string& cid);
	private:
		int32 delConnAll_();
		int32 startCtl();
		int32 stopCtl();
		int32 run();
		int32 onStopAndWait();
		static int32 workThreadProcess(EventLoop* el);
		int32 processOps();
		int32 processTimers(struct timeval& tv);
	private:
		SOCKET m_ctlfdr;
		SOCKET m_ctlfdw;
		THREADHANDLE m_thread;
		bool m_run;
		MUTEX m_ops_mtx;
		loop_buf m_ops;

		typedef std::map<std::string, CliConn*> CliConnMap;
		CliConnMap m_conns;
	};
}
#endif