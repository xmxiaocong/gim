#include "dispatcher.h"
#include "msg_head.h"
#include "logic_common.h"
#include "connect_server.pb.h"
#include "net/ef_event_loop.h"
#include <sstream>

namespace gim{



	int Dispatcher::addConnectServer(int id, 
		int conid){
		m_con_srv_map[id] = conid;
		return 0;
	}

	int Dispatcher::delConnectServer(int id){
		m_con_srv_map.erase(id);
		return 0;
	}	


	int Dispatcher::getConnectServer(int id, 
		int& conid){
		map<int, int>::iterator it = m_con_srv_map.find(id);
		if(it != m_con_srv_map.end()){
			conid = it->second;
			return 0;
		}
		return CONNECT_SERVER_OFFLINE;
	}

	static string _constructRequest(const string& sessid, int svtype, 
		const string& sn, int status, const string& payload,
		const string& callback){
		string msg;
		head h;
		h.magic = MAGIC_NUMBER;
		string body;
		h.cmd = SERVICE_REQ;
		constructServiceRequest("", sessid, svtype, sn, payload, 
				callback, body);
		constructPacket(h, body, msg);
		return msg;
	}
	
	static string _constructResponse(const string& sessid, int svtype, 
		const string& sn, int status, const string& payload, 
		const string& callback){
		string msg;
		head h;
		h.magic = MAGIC_NUMBER;
		string body;
		h.cmd = SERVICE_RESP;
		constructServiceResponse("", sessid, status, svtype, sn, 
				payload, callback, body);
		constructPacket(h, body, msg);
		return msg;
	}

	int Dispatcher::sendServiceRequestToClient(const string& cid, int svtype,
		const string& sn, const string& payload, const string& callback){
		return sendServicePacket(cid, svtype, sn, 0, payload, 
			callback, _constructRequest);
	}

	int Dispatcher::sendServiceResponseToClient(const string& cid, int svtype,
		const string& sn, int status, const string& payload, 
		const string& callback){
		return sendServicePacket(cid, svtype, sn, 0, payload, 
			callback, _constructResponse);
	}

	int Dispatcher::sendServicePacket(const string& cid, int svtype, 
		const string& sn, int status, const string& payload,
		const string& callback, ConstructPack cp){
		int ret = 0;
		vector<Sess> ss;
		ret = m_sesscache->getSession(cid, ss);

		if(!ss.size()){
			return CID_OFFLINE;
		}

		vector<Sess>::iterator it = ss.begin(); 

		for(; it != ss.end(); ++it){
			int conid = 0;
			ret = getConnectServer(it->svid(), conid);
			if(ret < 0){
				continue;
			}

			if(ret < 0){
				return ret;
			}
			string msg = (*cp)(it->sessid(), svtype, sn, 
				status, payload, callback);
			ret = m_evlp->sendMessage(conid, msg);
	
		}
		//std::cout << "m_last_error:" << m_last_error << std::endl;
		return ret;
	}


};
