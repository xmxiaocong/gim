#include "dispatcher.h"
#include "msg_head.h"
#include "logic_common.h"
#include "connect_server.pb.h"
#include "net/ef_event_loop.h"
#include <sstream>

namespace gim{



	int Dispatcher::addConnectServer(int id, 
		EventLoop* l, int conid){
		con_srv_t c;
		c.l = l;
		c.conid = conid;
		m_con_srv_map[id] = c;
		return 0;
	}

	int Dispatcher::delConnectServer(int id){
		m_con_srv_map.erase(id);
		return 0;
	}	


	int Dispatcher::getConnectServer(int id, 
		EventLoop*& l, int& conid){
		map<int, con_srv_t>::iterator it = m_con_srv_map.find(id);
		if(it != m_con_srv_map.end()){
			l = it->second.l;
			conid = it->second.conid;
			return 0;
		}
		return CONNECT_SERVER_OFFLINE;
	}

	static string _constructRequest(const string& sessid, int svtype, 
		const string& sn, int status, const string& payload){
		string msg;
		head h;
		h.magic = MAGIC_NUMBER;
		string body;
		h.cmd = SERVICE_REQ;
		constructServiceRequest(sessid, 
				svtype, sn, payload, body);
		constructReqPacket(h, body, msg);
		return msg;
	}
	
	static string _constructResponse(const string& sessid, int svtype, 
		const string& sn, int status, const string& payload){
		string msg;
		head h;
		h.magic = MAGIC_NUMBER;
		string body;
		h.cmd = SERVICE_RESP;
		constructServiceResponse(sessid, 
				status, svtype, sn, payload, body);
		constructReqPacket(h, body, msg);
		return msg;
	}

	int Dispatcher::sendServiceRequest(EventLoop* l, const string& cid, int svtype,
		const string& sn, const string& payload){
		return sendServicePacket(l, cid, svtype, sn, 0, payload, _constructRequest);
	}
	int Dispatcher::sendServiceResponse(EventLoop* l, const string& cid, int svtype,
		const string& sn, int status, const string& payload){
		return sendServicePacket(l, cid, svtype, sn, 0, payload, _constructResponse);
	}

	int Dispatcher::sendServicePacket(EventLoop* l,
		const string& cid, int svtype, 
		const string& sn, int status, const string& payload,
		ConstructPack cp){
		int ret = 0;
		vector<Sess> ss;
		m_last_error.clear();
		stringstream os;	
		ret = m_sesscache->getSession(cid, ss);

		if(!ss.size()){
			return CID_OFFLINE;
		}

		vector<Sess>::iterator it = ss.begin(); 

		for(; it != ss.end(); ++it){
			EventLoop* conl = NULL;
			int conid = 0;
			ret = getConnectServer(it->svid(), conl ,conid);
			if(ret < 0){
				continue;
			}

			if(ret < 0){
				return ret;
			}
			string msg = (*cp)(it->sessid(), svtype, sn, status, payload);

			if(l == conl){
				ret = conl->sendMessage(conid, msg);
			}else{
				ret = conl->asynSendMessage(conid, msg);
			}
	
		}
		//std::cout << "m_last_error:" << m_last_error << std::endl;
		return ret;
	}




};
