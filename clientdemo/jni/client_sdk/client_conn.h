#ifndef _CONNECTION_H_
#define _CONNECTION_H_
#include "common/ef_btype.h"
#include <string>
#include <set>
#include <map>
#include "msg_head.h"
#include "common/ef_sock.h"
#include "common/ef_loop_buf.h"
#include "common/ef_utility.h"
#include "proto/connect_server.pb.h"
#include "opbase.h"

using namespace ef;

namespace gim
{
	struct TimerKey
	{
		TimerKey(const std::string& _id, const timeval& tv)
			:id(_id), deadline(tv)
		{
		}
		TimerKey(const std::string& _id, int32 second)
			:id(_id)
		{
			gettimeofday(&deadline, NULL);
			deadline.tv_sec += second;
		}
		~TimerKey(){};
		bool operator < (const TimerKey& right) const
		{
			return tv_cmp(this->deadline, right.deadline) < 0;
		}
		std::string id;
		timeval deadline;
	};

	class EventLoop;
	class CliConn
	{
	public:
		CliConn(EventLoop *lp);
		virtual ~CliConn();
	public:
		SOCKET getfd() const
		{
			return m_fd;
		}

		void setCliver(const std::string& cliver);
		int32 getDeviceType();
		void setCid(const std::string& cid);
		void setPwd(const std::string& pwd);
		void setEnc(int32 enc);
		const std::string& getCid();
		int32 setSrvAddr(const std::string& ip, int32 port);
		int32 onDisconnect(bool notify, int code);
		int32 OnLoginFail(int code);
		int32 publish(const std::string& json);
		int32 connectAndLogin();

		int32 processTimers(const struct timeval& tnow, struct timeval& tv);
		int32 addTimer(const std::string& id, SmartOp& handle);
		int32 findAndDelTimer(const std::string& id, SmartOp& handle);
		int32 handleRead();

		int32 sendRequest(const std::string& sn, int32 cmd, int32 type, const std::string& payload);
		int32 sendPacket(int32 cmd, const std::string& body);
	private:
		int32 sendResponse(int32 status, const std::string& sn, int32 type, const std::string& payload);
		int32 connectServer();
		int32 login();
		void setStatus(int32 status, int32 code = 0, bool notify=false);
		int32 send_(const std::string& m);
		int32 addSrvaddr(const std::string& ip, int32 port);
		void closefd();
		void asynDestroy();
	private:
		virtual int32 handlePacket(const head& h, const std::string& body);
		int32 handleLoginResponse(const std::string& resp);
		int32 handleRedirectResponse(const std::string& resp);
		int32 handleResetTimeRespone(const std::string& resp);
		int32 handleServiceRequest(const std::string& req);
		int32 handleServiceResponse(const std::string& resp);
		int32 handleKeepAliveResp();
		int32 handlePeerPacket(const std::string& sn, const std::string& payload);
		int32 addKeepaliveTimer();
	private:
		SOCKET m_fd;
		loop_buf m_buf;

		//attrib
		int32 m_ver;
		std::string m_cli_ver;
		int32 m_devicetype;

		std::string m_cid;
		std::string m_pwd;
		int32 m_enc; // 0:raw data, 1:encode
		
		int32 m_loginStatus;
		int32 m_loginStatusCode;
		std::string m_sessid;
		int64 m_login_time;

		typedef struct _AddrItem
		{
			_AddrItem(const std::string& _ip, int32 _port)
				:ip(_ip), port(_port)
			{
			}
			std::string ip;
			int32 port;
		}AddrItem;
		typedef std::vector<AddrItem> AddrList;
		AddrList m_svrlist;

		typedef std::map<TimerKey, SmartOp> TimerList;
		TimerList m_timers;

		EventLoop* m_evlp;
	};
}
#endif
