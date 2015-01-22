#ifndef _CLIENT_H_
#define  _CLIENT_H_

#include "eventloop.h"

namespace gim
{
	class Client
	{
	public:
		Client(){};
		~Client(){};

		std::string getSN();

		int32 init();
		int32 login(const std::string& srvip, int32 srvport, const std::string& cid, const std::string& pwd, int32 enc, const std::string& version);
		int32 stop();
		int32 disconnect(const std::string& cid);
		int32 sendPeerMessage(const std::string& cid, const std::string& sn, const std::string& peercid, const std::string& data);
		virtual int handleMessage(const std::string& msg);
	private:
		static int eventLoopMsgRoutine(void* cli, const std::string& msg);
		EventLoop m_evlp;
		int64 m_sn;
	};
}

#endif //_CLIENT_H_
