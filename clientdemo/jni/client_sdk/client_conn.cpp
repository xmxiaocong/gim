#include "client_conn.h"
#include "client_log.h"
#include "logic_common.h"
#include "proto/connect_server.pb.h"
#include "json/json.h"
#include "service_type.h"
#include "proto/peer_server.pb.h"
#include <errno.h>
#include "common/ef_md5.h"
#include "ops.h"
#include "err_no.h"
#include "eventloop.h"

namespace gim
{
	CliConn::CliConn(EventLoop* lp)
		:m_fd(INVALID_SOCKET),
		m_login_time(0),
		m_loginStatus(STATUS_DISCONNECT),
		m_devicetype(0),
		m_enc(0),
		m_evlp(lp)
	{
		assert(m_evlp);
	}
	CliConn::~CliConn()
	{
	}
	void CliConn::setCliver(const std::string& cliver)
	{
		m_cli_ver = cliver;
	}
	int32 CliConn::getDeviceType()
	{
		return m_devicetype;
	}
	void CliConn::setCid(const std::string& cid)
	{
		m_cid = cid;
	}
	void CliConn::setPwd(const std::string& pwd)
	{
		m_pwd = pwd;
	}
	void CliConn::setEnc(int32 enc)
	{
		m_enc = enc;
	}
	const std::string& CliConn::getCid()
	{
		return m_cid;
	}
	int32 CliConn::setSrvAddr(const std::string& ip, int32 port)
	{
		return addSrvaddr(ip, port);
	}
	int32 CliConn::addSrvaddr(const std::string& ip, int32 port)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "CliConn::addSrvaddr %s:%d", ip.c_str(), port);
		m_svrlist.push_back(AddrItem(ip, port));
		return 0;
	}
	void CliConn::closefd()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::closefd", m_cid.c_str());
		if (m_fd != INVALID_SOCKET)
		{
			closesocket(m_fd);
			m_fd = INVALID_SOCKET;
		}
	}
	void CliConn::asynDestroy()
	{
		DelConnOp* op = new DelConnOp(getCid());
		m_evlp->asynAddOp((Op*)op);
	}
	int32 CliConn::connectServer()
	{
		while (!m_svrlist.empty())
		{
			AddrItem item = m_svrlist.back();
			SDK_LOG(LOG_LEVEL_TRACE, "connect %s:%d", item.ip.c_str(), item.port);
			m_fd = tcp_connect(item.ip.c_str(), item.port, "", 0);
			if (m_fd == INVALID_SOCKET)
			{
				SDK_LOG(LOG_LEVEL_ERROR, "connect fail");
				m_svrlist.pop_back();
			}
			else
			{
				break;
			}
		}

		if (m_fd == INVALID_SOCKET)
		{
			OnLoginFail(MY_NETWORK_ERROR);
			return -1;
		}
		return 0;
	}
	int32 CliConn::onDisconnect(bool notify, int code)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::onDisconnect", m_cid.c_str());
		for (TimerList::reverse_iterator rit = m_timers.rbegin(); rit != m_timers.rend(); ++rit)
		{
			if (rit->second.get())
			{
				rit->second->OnCancel(this);
			}
		}
		closefd();
		setStatus(STATUS_DISCONNECT, code,true);
		asynDestroy();
		return 0;
	}
	int32 CliConn::OnLoginFail(int code)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::OnLoginFail", m_cid.c_str());
		closefd();
		setStatus(STATUS_LOGIN_FAIL, code, true);
		asynDestroy();
		return 0;
	}
	int32 CliConn::publish(const std::string& json)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "[cid=%s] publish: %s", m_cid.c_str(), json.c_str());
		m_evlp->publish(json);
		return 0;
	}

	void CliConn::setStatus(int32 status, int32 code, bool notify)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "[cid=%s] setStatus before=%d, now=%d, code=%d, notify=%d", m_cid.c_str(), m_loginStatus, status, code, int32(notify));
		if (status != m_loginStatus)
		{
			m_loginStatus = status;
			m_loginStatusCode = code;

			if (notify)
			{
				Json::FastWriter w;
				Json::Value v;
				v[JKEY_CID] = getCid();
				v[JKEY_MSG_TYPE] = MSG_TYPE_LOGIN_STATUS_CHANGE;
				v[JKEY_MSG_STATUS] = code;
				v[JKEY_MSG_LOGIN_STATUS] = status;
				publish(w.write(v).c_str());
			}
		}
	}
	int32 CliConn::connectAndLogin()
	{
		if ((m_loginStatus != STATUS_DO_LOGIN) 
			&& (m_loginStatus != STATUS_LOGIN) 
			&& connectServer() == 0)
		{
			return login();
		}
		return -1;
	}
	int32 CliConn::login()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "CliConn::login");
		setStatus(STATUS_DO_LOGIN);

		int32 ret = 0;
		m_login_time = gettime_ms();
		LoginRequest lgr;
		lgr.set_cid(getCid());
		lgr.set_enc(m_enc);
		lgr.set_version(m_cli_ver);
		lgr.set_time(m_login_time);

		std::string s = m_cid + m_pwd + m_cli_ver + ef::itostr(m_login_time);
		std::string tk;
		ef::MD5Hex(tk, (const ef::uint8*)s.data(), s.size());
		lgr.set_token(tk);
		std::string body;
		lgr.SerializeToString(&body);

		std::string pack;
		head h;
		h.magic = 0x20140417;
		h.cmd = LOGIN_REQ;
		constructReqPacket(h, body, pack);
		ret = send_(pack);
		return ret >= 0 ? 0 : -1;
	}
	int32 CliConn::handleRead()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::handleRead", m_cid.c_str());
		int32	ret = 0;
		char*	tmpbuf = NULL;
		int32	wantlen = 0;
		int32	actrcv = 0;
		int32	totalrcv = 0;
		while (true)
		{
			actrcv = 0;
			ret = 0;
			wantlen = m_buf.next_part_len();
			if (wantlen <= 0)
			{
				m_buf.resize(m_buf.capacity() + 16 * 1024);
				wantlen = m_buf.next_part_len();
			}

			tmpbuf = (char*)m_buf.next_part();

			ret = tcp_nb_receive(m_fd, tmpbuf, wantlen, &actrcv);
			if (ret < 0)
			{
				SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, recvLoop recv  error=%d, %d", m_cid.c_str(), ret, sock_errno);
				return -1;
			}
			if (actrcv == 0)
			{
				break;
			}
			totalrcv += actrcv;
			m_buf.write(NULL, actrcv);
		};

		while (true)
		{
			head h;
			if (m_buf.peek((uint8*)&h, sizeof(h)) < sizeof(h))
			{
				break;
			}
			h.magic = ntohl(h.magic);
			h.len = ntohl(h.len);
			h.cmd = ntohl(h.cmd);

			if (m_buf.size() < h.len)
			{
				break;
			}

			SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, recvLoop cmd=%d, len=%d", m_cid.c_str(), h.cmd, h.len);
			m_buf.read(NULL, sizeof(h));
			std::string body;
			body.resize(h.len - sizeof(h));
			m_buf.read((uint8*)body.data(), h.len - sizeof(h));
			handlePacket(h, body);
		}
		return 0;
	}
	int32 CliConn::send_(const std::string& m)
	{
		if (m.empty() || getfd() == INVALID_SOCKET)
		{
			return -1;
		}
		int32 ack = 0;
		int32 ret = tcp_send(getfd(), m.data(), m.size(), &ack);
		if (ret <= 0)
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, CliConn::send fail",m_cid.c_str());
			return -1;
		}
		return 0;
	}

	int32 CliConn::handlePacket(const head& h, const std::string& body)
	{
		int32 ret;
		switch (h.cmd)
		{
		case KEEPALIVE_RESP:
			ret = handleKeepAliveResp();
			break;
		case LOGIN_RESP:
			ret = handleLoginResponse(body);
			break;
		case REDIRECT_RESP:
			ret = handleRedirectResponse(body);
			break;
		case SET_TIME_RESP:
			ret = handleResetTimeRespone(body);
			break;
		case SERVICE_CMD_REQ:
			ret = handleServiceRequest(body);
			break;
		case SERVICE_CMD_RESP:
			ret = handleServiceResponse(body);
			break;
		}
		return ret;
	}
	int32 CliConn::handleLoginResponse(const std::string& resp)
	{
		SmartOp op(NULL);
		findAndDelTimer(LOGIN_OP_SN, op);

		LoginResponse lgresp;
		if (!lgresp.ParseFromArray(resp.data(), resp.size()))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleLoginResponse parse probuf error", m_cid.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}
		int32 status = lgresp.status();
		if (status >= 0)
		{
			m_sessid = lgresp.sessid();
			setStatus(STATUS_LOGIN, status, true);

			//offline peer msg
			SmartOp offlinePeerOp(new GetOfflinePeerMsgOp(itostr(gettime_ms()), getCid()));
			offlinePeerOp->process(this);
			if (offlinePeerOp.release() > 0 && offlinePeerOp.get())
			{
				addTimer(offlinePeerOp->getSN(), offlinePeerOp);
			}

			//keepalive op
			addKeepaliveTimer();
		}
		else
		{
			m_svrlist.pop_back();
			if (m_svrlist.empty())
			{
				SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleLoginResponse fail ret=%d", m_cid.c_str(), status);
				OnLoginFail(status);
			}
			else
			{
				connectAndLogin();
			}
		}
		return 0;
	}
	int32 CliConn::handleRedirectResponse(const std::string& resp)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, handleRedirectResponse", m_cid.c_str());
		RedirectResponse rdresp;
		if (!rdresp.ParseFromString(resp))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleRedirectResponse parse probuf error", m_cid.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		};
		closefd();
		m_svrlist.pop_back();
		for (size_t i = 0; i < rdresp.addrs_size(); ++i)
		{
			const Address& a = rdresp.addrs(i);
			addSrvaddr(a.ip(), a.port());
		}

		int32 ret = connectAndLogin();
		return ret;
	}
	int32 CliConn::handleResetTimeRespone(const std::string& reps)
	{
		SetTimeResponse respone;
		if (!respone.ParseFromString(reps))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleResetTimeRespone parse error",m_cid.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}

		m_login_time = respone.server_time();
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, handleResetTimeRespone get time=%s", m_cid.c_str(), itostr(m_login_time).c_str());
		int32 ret = login();
		return ret;
	}
	int32 CliConn::handleServiceRequest(const std::string& req)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::handleServiceRequest", m_cid.c_str());
		ServiceRequest svreq;
		if (!svreq.ParseFromArray(req.data(), req.size()))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleServiceRequest parse fail", m_cid.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}
		int32 srvtype = svreq.svtype();
		switch (srvtype)
		{
		case SERVICE_PEER:
			return  handlePeerPacket(svreq.sn(), svreq.payload());
		default:
			return MY_PROBUF_FORMAT_ERROR;
		}
	}
	int32 CliConn::handlePeerPacket(const std::string& sn, const std::string& payload)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::handlePeerPacket sn=%s", m_cid.c_str(), sn.c_str());

		Json::FastWriter w;
		Json::Value v;

		v[JKEY_CID] = getCid();
		v[JKEY_MSG_TYPE] = MSG_TYPE_PEER;
		v[JKEY_MSG_SN] = sn;

		PeerPacket peerpack;
		int32 ret = MY_OK;

		if (!peerpack.ParseFromArray(payload.data(), payload.size()))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, CliConn::handlePeerPacket ParseFromArray fail sn=%s", m_cid.c_str(), sn.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}

		if (peerpack.cmd() != 110)
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, CliConn::handlePeerPacket sn=%s pack_type=%d, != 110", m_cid.c_str(), sn.c_str(), peerpack.cmd());
			return MY_PROBUF_FORMAT_ERROR;
		}
		if (!peerpack.has_send_peer_msg_req())
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, CliConn::handlePeerPacket sn=%s pack has_send_peer_msg_req() return false", m_cid.c_str(), sn.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}

		const SendPeerMessageRequest& preq = peerpack.send_peer_msg_req();
		const Message& msg = preq.msg();

		v[JKEY_MSG_STATUS] = STATUS_OK;

		v[JKEY_MSG_ID] = itostr(msg.id());
		v[JKEY_MSG_TIME] = itostr(msg.time());
		v[JKEY_MSG_FROM] = msg.from();
		v[JKEY_MSG_TO] = msg.to();
		v[JKEY_MSG_DATA] = msg.data();
		publish(w.write(v).c_str());

		// send respone
		{
			PeerPacket resppack;
			resppack.set_cmd(115);
			RecvPeerMessageResponse* pshresp = resppack.mutable_recv_peer_msg_resp();
			Message* respmsg = pshresp->mutable_msg();
			respmsg->set_from(msg.from());
			respmsg->set_to(msg.to());
			respmsg->set_time(msg.time());
			respmsg->set_id(msg.id());
			std::string payload;
			resppack.SerializeToString(&payload);
			sendResponse(STATUS_OK, sn, SERVICE_PEER, payload);
		}
		return ret;
	}
	int32 CliConn::handleServiceResponse(const std::string& resp)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::handleServiceResponse", m_cid.c_str());
		int32 ret = 0;
		ServiceResponse svresp;
		if (!svresp.ParseFromArray(resp.data(), resp.size()))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "cid=%s, handleServiceResponse parse fail", m_cid.c_str());
			return MY_PROBUF_FORMAT_ERROR;
		}

		int32 status = svresp.status();
		std::string sn = svresp.sn();
		SmartOp op(NULL);
		if (findAndDelTimer(sn, op) == 0 && op.get())
		{
			ret = op->onRespone(this, status, svresp.payload());
			if (op.release() > 0)
			{
				addTimer(op->getSN(), op);
			}
		}
		else
		{
			SDK_LOG(LOG_LEVEL_WARN, "cid=%s, CliConn::handleServiceResponse op %s not found", m_cid.c_str(), sn.c_str());
		}
		return ret;
	}
	int32 CliConn::handleKeepAliveResp()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::handleKeepAliveResp", getCid().c_str());
		addKeepaliveTimer();
		return 0;
	}
	int32 CliConn::addKeepaliveTimer()
	{
		SmartOp op(new KeepaliveOp(getCid()));
		addTimer(op->getSN(), op);
		return 0;
	}
	int32 CliConn::addTimer(const std::string& id, SmartOp& sp)
	{
		if (sp.get())
		{
			m_timers.insert(TimerList::value_type(TimerKey(id, sp->getTimeout()), SmartOp(sp.get())));
			SDK_LOG(LOG_LEVEL_TRACE, "CliConn::addTimer op=%s", id.c_str());
		}
		return 0;
	}
	int32 CliConn::findAndDelTimer(const std::string& id, SmartOp& sp)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "cid=%s, CliConn::findAndDelTimer sp=%s", m_cid.c_str(), id.c_str());
		for (TimerList::reverse_iterator rit = m_timers.rbegin(); rit != m_timers.rend(); ++rit)
		{
			if (rit->first.id == id)
			{
				sp.reset(rit->second.get());
				m_timers.erase((++rit).base());
				return 0;
			}
		}
		return -1;
	}
	int32 CliConn::processTimers(const struct timeval& tnow, struct timeval& tv)
	{
		std::vector<SmartOp> ops;
		for (TimerList::iterator it = m_timers.begin(); it != m_timers.end();)
		{
			TimerKey key = it->first;
			if (tv_cmp(key.deadline, tnow) > 0)
			{
				timeval tvtemp = tv_diff(key.deadline, tnow);
				if (tv_cmp(tv, tvtemp) < 0)
				{
					tv = tvtemp;
				}
				it++;
			}
			else
			{
				//SDK_LOG(LOG_LEVEL_TRACE, "op %s time out", key.id.c_str());
				ops.push_back(it->second);
				m_timers.erase(it++);
			}
		}

		for (std::vector<SmartOp>::iterator it = ops.begin(); it != ops.end(); it++)
		{
			if (it->get())
			{
				it->get()->OnTimeout(this);
			}
		}
		return 0;
	}
	int32 CliConn::sendRequest(const std::string& sn, int32 cmd, int32 type, const std::string& payload)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "sendRequest sn=%s, cmd=%d, type=%d", sn.c_str(), cmd, type);
		int32 ret = 0;
		std::string body;
		constructServiceRequest(m_sessid, type, sn, payload, body);
		ret = sendPacket(cmd, body);
		return ret;
	}
	int32 CliConn::sendResponse(int32 status, const std::string& sn, int32 type, const std::string& payload)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "sendResponse sn=%s, type=%d", sn.c_str(), type);
		int32 ret = 0;
		std::string body;
		constructServiceResponse(m_sessid, status, type, sn, payload, body);
		std::string packet;
		constructRespPacket(SERVICE_CMD_RESP, body, packet);
		ret = send_(packet);
		return ret;
	}
	int32 CliConn::sendPacket(int32 cmd, const std::string& body)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "sendPacket");
		int32 ret = 0;
		std::string packet;
		head h;
		h.magic = 0x20140417;
		h.cmd = cmd;
		constructReqPacket(h, body, packet);

		ret = send_(packet);
		return ret;
	}
}
