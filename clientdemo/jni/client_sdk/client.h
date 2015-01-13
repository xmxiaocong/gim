#ifndef _CLIENT_H_
#define  _CLIENT_H_
#include "eventloop.h"
#include "client_conn.h"
#include <vector>
#include <string>
#include "common/ef_utility.h"
#include "client_def.h"

namespace gim
{
	class Client
	{
	public:
		Client(){};
		~Client(){};
		int32 init();
		int32 login(const std::string& srvip, int32 srvport, const std::string& cid, const std::string& pwd, int32 enc, const std::string& version);
		int32 stop();
		int32 disconnect(const std::string& cid);
		int32 sendPeerMessage(const std::string& cid, const std::string& sn, const GPeerMessage& msg);
		virtual int handleMessage(const std::string& msg);
		std::string getSN()
		{
			return itostr(m_sn++);
		}
	private:
		static int eventLoopMsgRoutine(void* cli, const std::string& msg);
		EventLoop m_evlp;
		int64 m_sn;
	};
}

#endif //_CLIENT_H_
