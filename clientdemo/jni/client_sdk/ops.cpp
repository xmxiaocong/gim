#include "ops.h"
#include "proto/connect_server.pb.h"
#include "msg_head.h"
#include "eventloop.h"
#include "logic_common.h"
#include "client_conn.h"
#include "client_log.h"
#include "proto/peer_server.pb.h"
#include "service_type.h"
#include "json/writer.h"
#include "err_no.h"

namespace gim
{
	// class LoginOp
	int32 LoginOp::init(const std::string& srvip, int32 srvport, const std::string& cliver, int32 enc, const std::string& pwd)
	{
		m_srvip = srvip;
		m_srvport = srvport;
		m_pwd = pwd;
		m_cliver = cliver;
		m_enc = enc;
		return 0;
	}
	
	int32 LoginOp::process(EventLoop* el)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "LoginImsOp::process");
		if (!el)
		{
			return -1;
		}
		CliConn* conn = el->findConn(getCid());
		if (!conn)
		{
			conn = el->addConn(getCid());
			conn->setCid(getCid());
			conn->setPwd(m_pwd);
			conn->setEnc(m_enc);
			conn->setCliver(m_cliver);
			conn->setSrvAddr(m_srvip, m_srvport);
		}
		else
		{
			SDK_LOG(LOG_LEVEL_ERROR, "LoginImsOp::process, connection %s exists", getCid().c_str());
			return -1;
		}

		int32 ret = conn->connectAndLogin();
		if (ret == 0)
		{
			increase_();
		}
		return ret;
	}
	int32 LoginOp::OnTimeout(CliConn* conn)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "LoginImsOp::OnTimeout");
		if (conn)
		{
			conn->OnLoginFail(MY_TIMEOUT);
		}

		return 0;
	}
	//KeepaliveOp
	int32 KeepaliveOp::process(CliConn* conn)
	{
		if (conn)
		{
			increase_();
			return 0;
		}
		return -1;
	}
	int32 KeepaliveOp::OnTimeout(CliConn* conn)
	{
		if (conn)
		{
			return conn->sendPacket(KEEPALIVE_REQ, "");
		}
		return -1;
	}
	// class DelConnOp
	int32 DelConnOp::process(EventLoop* el)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "DelConnOp::process");
		if (el)
		{
			return el->delConn(getCid());
		}
		return -1;
	}
	// class DisconnectOp
	int32 DisconnectOp::process(EventLoop* el)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "DisconnectOp::process");
		CliConn* conn = el ? el->findConn(getCid()) : NULL;
		if (conn)
		{
			conn->onDisconnect(true, 0);
			return 0;
		}
		return -1;
	}
	// class SendPeerMessageOp
	int32 SendPeerMessageOp::process(CliConn* conn)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "SendPeerMessageOp::process");
		if (conn)
		{
			PeerPacket peerpack;
			peerpack.set_cmd(110);
			SendPeerMessageRequest* spreq = peerpack.mutable_send_peer_msg_req();
			Message* pm = spreq->mutable_msg();
			pm->set_to(m_msg.to);
			pm->set_from(m_msg.from);
			pm->set_data(m_msg.data);
			std::string payload;
			peerpack.SerializeToString(&payload);
			int32 ret = conn->sendRequest(getSN(), SERVICE_CMD_REQ, SERVICE_PEER, payload);
			if (ret == MY_OK)
			{
				increase_();
				return 0;
			}
		}
		return -1;
	}
	int32 SendPeerMessageOp::onRespone(CliConn* conn, int32 status, const std::string& payload)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "SendPeerMessageOp::onRespone");
		if (!conn)
		{
			return -1;
		}

		Json::FastWriter w;
		Json::Value v;

		int32 ret = 0;
		v[JKEY_CID] = getCid();
		v[JKEY_MSG_TYPE] = MSG_TYPE_SEND_PEER_RESP;
		v[JKEY_MSG_STATUS] = status;
		v[JKEY_MSG_SN] = getSN();

		if (status >= 0)
		{
			ret = 0;
			PeerPacket peerpacket;
			if (!peerpacket.ParseFromArray(payload.data(), payload.size()))
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray fail");
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}

			if (111 != peerpacket.cmd())
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray cmd=%d, != 111", peerpacket.cmd());
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}
			if (!peerpacket.has_send_peer_msg_resp())
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray has_send_peer_msg_resp() return false");
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}
			const SendPeerMessageResponse& pcresp = peerpacket.send_peer_msg_resp();
			const Message& msg = pcresp.msg();

			v[JKEY_MSG_ID] = itostr(msg.id());
			v[JKEY_MSG_TIME] = itostr(msg.time());
			v[JKEY_MSG_FROM] = msg.from();
			v[JKEY_MSG_TO] = msg.to();
			v[JKEY_MSG_DATA] = msg.data();
		}

parse_end:
		conn->publish(w.write(v).c_str());
		return ret;
	}
	int32 SendPeerMessageOp::OnTimeout(CliConn* conn)
	{
		if (conn)
		{
			Json::FastWriter w;
			Json::Value v;

			v[JKEY_CID] = getCid();
			v[JKEY_MSG_TYPE] = MSG_TYPE_SEND_PEER_RESP;
			v[JKEY_MSG_STATUS] = REQUEST_TIME_OUT;
			v[JKEY_MSG_SN] = getSN();
			conn->publish(w.write(v).c_str());
		}
	}
	// class GetOfflinePeerMsgOp
	int32 GetOfflinePeerMsgOp::process(CliConn* conn)
	{
		if (conn)
		{
			sendRequest_(conn, 0, GET_OFFLINE_PEER_MSG_SIZE_EACH);
		}
	}
	int32 GetOfflinePeerMsgOp::sendRequest_(CliConn* conn, int64 startid, int32 len)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "GetOfflinePeerMsgOp::sendRequest cid:%s sn:%s startid:%s size:%d", (conn ? conn->getCid().c_str() : "null"), getSN().c_str(), itostr(startid).c_str(), len);
		if (conn)
		{
			PeerPacket peerpack;
			peerpack.set_cmd(112);
			GetPeerMessageRequest* phcreq = peerpack.mutable_get_peer_msg_req();
			phcreq->set_cid(conn->getCid());
			phcreq->set_start_msgid(startid);
			phcreq->set_count(len);
			std::string payload;
			peerpack.SerializeToString(&payload);
			int32 ret = conn->sendRequest(getSN(), SERVICE_CMD_REQ, SERVICE_PEER, payload);
			if (ret == MY_OK)
			{
				increase_();
			}
			return ret;
		}
		return -1;
	}
	int32 GetOfflinePeerMsgOp::onRespone(CliConn* conn, int32 status, const std::string& payload)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "GetOfflinePeerMsgOp::onRespone cid=%s, status=%d,sn=%s", (conn ? conn->getCid().c_str() : "null"), status, getSN().c_str());
		if (!conn)
		{
			return -1;
		}

		Json::FastWriter w;
		Json::Value v;

		v[JKEY_CID] = getCid();
		v[JKEY_MSG_TYPE] = MSG_TYPE_OFFLINE_PEER_RESP;
		v[JKEY_MSG_SN] = getSN();
		v[JKEY_MSG_STATUS] = status;
		int32 ret = MY_OK;
		if (status >= 0)
		{
			PeerPacket peerpacket;
			int64 max_msg_id = 0;
			if (!peerpacket.ParseFromArray(payload.data(), payload.size()))
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray fail");
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}

			if (113 != peerpacket.cmd())
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray type=%d, != 113", peerpacket.cmd());
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}

			if (!peerpacket.has_get_peer_msg_resp())
			{
				SDK_LOG(LOG_LEVEL_ERROR, "PeerPacket ParseFromArray has_get_peer_msg_resp() return false");
				ret = MY_PROBUF_FORMAT_ERROR;
				goto parse_end;
			}

			const GetPeerMessageResponse& pcresp = peerpacket.get_peer_msg_resp();
			Json::Value vmsg;
			for (size_t i = 0; i < pcresp.msgs_size(); ++i)
			{
				const Message& msg = pcresp.msgs(i);

				vmsg[JKEY_MSG_ID] = itostr(msg.id());
				vmsg[JKEY_MSG_TIME] = itostr(msg.time());
				vmsg[JKEY_MSG_TO] = msg.to();
				vmsg[JKEY_MSG_FROM] = msg.from();
				vmsg[JKEY_MSG_DATA] = msg.data();
				v[JKEY_MSG_ARRAY].append(vmsg);
				if (msg.id() > max_msg_id)
				{
					max_msg_id = msg.id();
				}
			}
			int64 lastmsgid = pcresp.last_msgid();
			int32 leftsize = (max_msg_id == 0) ? 0 : (lastmsgid - max_msg_id);
			if (max_msg_id > 0)
			{
				sendRequest_(conn, max_msg_id+1, GET_OFFLINE_PEER_MSG_SIZE_EACH);
			}
			v[JKEY_MSG_OFFLINE_LEFTSIZE] = leftsize;
		}
	parse_end:
		conn->publish(w.write(v).c_str());
		return ret;
	}
}//end of namespace
#endif // !_EVENT_LOOP_OPERATION_H_

