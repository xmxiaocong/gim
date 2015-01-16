#ifndef _OP_BASE_H_
#define  _OP_BASE_H_

#include <string>
#include <common/ef_btype.h>
using namespace ef;
namespace gim
{
	class EventLoop;
	class CliConn;

#define OP_DEFAULT_TIMEOUT 30

	// ops do process in eventloop, and wait for respone in connection
	class Op
	{
		friend class SmartOp;
	public:
		Op(const std::string& sn, const std::string& cid);
		virtual ~Op(){};
		virtual int32 process(EventLoop* el);
		virtual int32 process(CliConn * conn);
		void setTimeout(int32 sec);
		int32 getTimeout() const;
		virtual int32 OnTimeout(CliConn* conn);
		virtual int32 OnCancel(CliConn* conn);
		virtual int32 onRespone(CliConn* conn, int32 status, const std::string& payload);
		std::string getSN() const;
		std::string getCid()const;
	protected:
		int32 increase_();
		int32 decrease_();
	private:
		volatile int32 m_ref;
		std::string m_sn;
		std::string m_cid;
		int32 m_timeout;
	};

	class SmartOp
	{
	public:
		SmartOp(Op* op);
		SmartOp(const SmartOp& oth);
		SmartOp& operator = (const SmartOp& right);
		~SmartOp();
		void reset(Op* op);
		int32 addref();
		int32 release();
		Op* get() const;
		Op* operator ->() const;
	protected:
		Op* m_op;
	private:
	};
}
#endif
