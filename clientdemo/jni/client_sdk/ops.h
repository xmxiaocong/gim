#ifndef _OPERATION_H_
#define  _OPERATION_H_

#include "opbase.h"
#include <string>
#include "common/ef_btype.h"
#include "client_def.h"


using namespace ef;
#define GET_OFFLINE_PEER_MSG_SIZE_EACH 20
#define GET_OFFLINE_PUSH_MSG_SIZE_EACH 20
#define KEEPALIVE_TIMEOUT 300

#define LOGIN_OP_SN "Login"
namespace gim
{
	class EventLoop;
	class CliConn;

	class LoginOp
		:public Op
	{
	public:
		LoginOp(const std::string& cid)
			:Op(LOGIN_OP_SN, cid),
			m_srvport(0),
			m_enc(0)
		{
		}
		~LoginOp(){};
		int32 init(const std::string& srvip, int32 srvport, const std::string& cliver, int32 enc, const std::string& pwd);
		virtual int32 process(EventLoop* el);
		virtual int32 OnTimeout(CliConn* conn);
	private:
		std::string m_srvip;
		int32 m_srvport;
		std::string m_pwd;
		std::string m_cliver;
		int32 m_enc;
	};

	class KeepaliveOp
		:public Op
	{
	public:
		KeepaliveOp(const std::string& cid)
			:Op(std::string("keepalive"), cid)
		{
			setTimeout(KEEPALIVE_TIMEOUT);
		}
		~KeepaliveOp(){};
		virtual int32 process(CliConn* conn);
		virtual int32 OnTimeout(CliConn* conn);
	};

	class DelConnOp
		:public Op
	{
	public:
		DelConnOp(const std::string& cid)
			:Op(std::string("delconnect"), cid)
		{
		}
		~DelConnOp(){};
		virtual int32 process(EventLoop* el);
	};
	class DisconnectOp
		:public Op
	{
	public:
		DisconnectOp(const std::string& cid)
			:Op(std::string("disconnect"), cid)
		{
		}
		~DisconnectOp(){};
		virtual int32 process(EventLoop* el);
	};

	class SendPeerMessageOp
		:public Op
	{
	public:
		SendPeerMessageOp(const std::string& sn, const std::string& cid)
			:Op(sn, cid)
		{
		}
		~SendPeerMessageOp(){};
		int32 init(const std::string& peercid, const std::string& data)
		{
			m_peerid = peercid;
			m_data = data;
			return 0;
		}
		virtual int32 process(CliConn* conn);
		virtual int32 onRespone(CliConn* conn, int32 status, const std::string& payload);
		virtual int32 OnTimeout(CliConn* conn);
	private:
		std::string m_peerid;
		std::string m_data;
	};

	class GetOfflinePeerMsgOp
		:public Op
	{
	public:
		GetOfflinePeerMsgOp(const std::string& sn, const std::string& cid)
			:Op(sn, cid)
		{
		}
		~GetOfflinePeerMsgOp(){};
		virtual int32 process(CliConn* conn);
		virtual int32 onRespone(CliConn* conn, int32 status, const std::string& payload);\
	private:
		int32 sendRequest_(CliConn* conn, int64 startid, int32 len);
	};

}
#endif