#include "client.h"
#include "ops.h"
#include "common/ef_utility.h"
namespace gim
{

	int32 Client::init()
	{
		m_sn = gettime_ms();
		m_evlp.setMsgCb(eventLoopMsgRoutine, (void*)this);
		m_evlp.startLoop();
		return 0;
	}

	int32 Client::login(const std::string& srvip, int32 srvport, const std::string& cid, const std::string& pwd, int32 enc, const std::string& version)
	{
		LoginOp* op = new LoginOp(cid);
		op->init(srvip, srvport, version, enc, pwd);
		return m_evlp.asynAddOp((Op*)op);
	}
	int32 Client::stop()
	{
		m_evlp.asynStop();
		return 0;
	}
	int32 Client::disconnect(const std::string& cid)
	{
		DisconnectOp* op = new DisconnectOp(cid);
		return m_evlp.asynAddOp((Op*)op);
	}
	int32 Client::sendPeerMessage(const std::string& cid, const std::string& sn, const GPeerMessage& msg)
	{
		SendPeerMessageOp* op = new SendPeerMessageOp(getSN(), cid);
		op->init(msg);
		return m_evlp.asynAddOp((Op*)op);
	}
	int Client::eventLoopMsgRoutine(void* cli, const std::string& msg)
	{
		return cli ? ((Client*)cli)->handleMessage(msg) : -1;
	}
	int Client::handleMessage(const std::string& msg)
	{
		return 0;
	}
}
