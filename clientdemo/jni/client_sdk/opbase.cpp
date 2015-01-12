#include "opbase.h"
#include "eventloop.h"
#include "client_conn.h"

namespace gim
{
	// class op
	Op::Op(const std::string& sn, const std::string& cid)
		:m_ref(0), m_sn(sn),
		m_cid(cid),
		m_timeout(OP_DEFAULT_TIMEOUT)
	{
	}
	int32 Op::process(EventLoop* el)
	{
		return el ? process(el->findConn(getCid())) : -1;
	}
	int32 Op::process(CliConn * conn)
	{
		return 0;
	}
	void Op::setTimeout(int32 sec)
	{
		m_timeout = sec;
	}
	int32 Op::getTimeout() const 
	{
		return m_timeout;
	}
	int32 Op::OnTimeout(CliConn* conn)
	{
		return 0;
	}
	int32 Op::OnCancel(CliConn* conn)
	{
		//SDK_LOG(LOG_LEVEL_TRACE, "op OnCancel sn=%s", getSN().c_str());
		return 0;
	}
	int32 Op::onRespone(CliConn* conn, int32 status, const std::string& payload)
	{
		return 0;
	}
	std::string Op::getSN() const
	{
		return m_sn;
	}
	std::string Op::getCid() const
	{
		return m_cid;
	}

	int32 Op::increase_()
	{
		++m_ref;
		//SDK_LOG(LOG_LEVEL_TRACE, "sn:%s, cid:%s increase ref=%d", getSN().c_str(), getCid().c_str(), m_ref);
		return m_ref;
	}

	int32 Op::decrease_()
	{
		--m_ref;
		//SDK_LOG(LOG_LEVEL_TRACE, "sn:%s, cid:%s decrease ref=%d", sn().c_str(), cid().c_str(), m_ref);
		return m_ref;
	}

	//SmartOp
	SmartOp::SmartOp(Op* op)
		:m_op(op)
	{
		addref();
	}
	SmartOp::SmartOp(const SmartOp& oth)
		: m_op(oth.get())
	{
		addref();
	}
	SmartOp& SmartOp::operator = (const SmartOp& right)
	{
		reset(right.get());
		return *this;
	}
	void SmartOp::reset(Op* op)
	{
		release();
		this->m_op = op;
		addref();
	}
	SmartOp::~SmartOp()
	{
		release();
	}
	int32 SmartOp::addref()
	{
		return m_op ? m_op->increase_() : -1;
	}
	int32 SmartOp::release()
	{
		if (m_op)
		{
			int32 count = m_op->decrease_();
			if (count <= 0)
			{
				delete m_op;
				m_op = NULL;
			}
			return count;
		}
		return -1;
	}
	Op* SmartOp::get() const
	{
		return m_op;
	}
	Op* SmartOp::operator ->() const
	{
		return get();
	}
}
