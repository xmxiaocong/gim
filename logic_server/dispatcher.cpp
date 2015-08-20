#include "dispatcher.h"
#include "msg_head.h"
#include "err_no.h"
#include "logic_common.h"
#include "server_conn.h"
#include "rpc_client.h"
#include "logic_server.h"
#include "connect_server.pb.h"
#include "net/ef_event_loop.h"
#include <sstream>

namespace gim{

using namespace std;
using namespace ef;

	int Dispatcher::addConnectServer(int id, 
		SvCon* con){
		m_svcons[id] = con;
		return 0;
	}

	int Dispatcher::delConnectServer(int id){
		m_svcons.erase(id);
		return 0;
	}	

	Dispatcher::Dispatcher(LogicServer* s, ef::EventLoop* l, DtSrc* src):
		m_loader(src), m_sv(s), m_evlp(l), m_sesscache(NULL), m_rpc_cli(NULL){
	}

	int Dispatcher::init(void* par){
		m_sesscache = SsChFactory::getSsChFactory()->getSessCache();
		m_rpc_cli = new RPCClient();
		return 0;
	}

	Dispatcher::~Dispatcher(){
		delete m_rpc_cli;
		//do not delete sesscache, because it will be delete on thread exit;
	}



	
	int Dispatcher::connectIPArray(SvCon* c, const vector<string>& a, 
		int port){
	
		int ret = 0;
	
		if(a.size() <= 0){
			return -1;
		}
	
		for(Json::UInt i = 0; i < a.size(); ++i){
			
			ret = c->connectTo(a[i], port);
			
			if(ret >= 0){
				c->setAddr(a[i], port);
				break;	
			}
		}
	
		return ret;
	
	}
	
	
	int Dispatcher::connectServer(const ServerStatus& s){
		int ret = 0;
		SvCon* c = m_sv->getSvConFactory()->createSvCon(NULL);
		LogicConfig conf = m_sv->getConfig();
		
		c->setConnectServerId(s.ID);
		c->setLogicServerId(conf.ID);
		c->setSvType(conf.Type);
		c->setReconnectSpan(conf.ReconnectSpanMS);
		c->setKeepAliveSpan(conf.KeepaliveSpanMS);
		c->setLogicServer(m_sv);
		
		int port = s.Ports[0].Port;
		vector<string> localIPs;
		vector<string> publicIPs;

		for(size_t i = 0; i < s.IPs.size(); ++i){
			if(isLocalIP(s.IPs[i])){
				localIPs.push_back(s.IPs[i]);
			}else{
				publicIPs.push_back(s.IPs[i]);
			}
		}
	
		ret = connectIPArray(c, localIPs, port);

		if(ret < 0){
			ret = connectIPArray(c, publicIPs, port);
		}
		
		m_evlp->asynAddConnection(c);
		
		return 0;
	}


	int Dispatcher::checkConnectServers(){
		bool l = m_loader.loadData();
		if(!l){
			return 0;
		}

		StatusMap smap = m_loader.getData();
		StatusMap::iterator itor = smap.begin();
		for(; itor != smap.end(); ++itor){
			if(getConnectServer(itor->first)){
				continue;
			}
			connectServer(itor->second);
		}

		//RPCClient reinit
		reinitRPCClient(smap);
		
		return 0;
	}

	SvCon* Dispatcher::getConnectServer(int id){
		map<int, SvCon*>::iterator it = m_svcons.find(id);
		if(it != m_svcons.end()){
			return it->second;
		}
		return NULL;
	}

	int Dispatcher::getClientLoginServer(const string& cid, vector<CliSess>& svs){
		int ret = 0;
		vector<Sess> ss;
		ret = m_sesscache->getSession(cid, ss);

		if(!ss.size()){
			return CID_OFFLINE;
		}

		vector<Sess>::iterator it = ss.begin(); 

		for(; it != ss.end(); ++it){
			SvCon* s = getConnectServer(it->consvid());
			if(!s){
				continue;
			}
			svs.push_back(CliSess(it->sessid(), s));
		}

		return svs.size();
	}

	int Dispatcher::reinitRPCClient(const ServerStatus& s){
		int ret = 0;
		int port = s.Ports[0].Port;
		vector<string> localIPs;
		vector<string> publicIPs;	
		for(size_t i = 0; i < s.IPs.size(); ++i){
			if(!isLocalIP(s.IPs[i])){
				continue;
			}
			ret = m_rpc_cli->init(s.IPs[i], port);
			if(ret >= 0){
				return ret;
			}
		}
	
		for(size_t j = 0; j < s.IPs.size(); ++j){
			if(isLocalIP(s.IPs[j])){
				continue;
			}
			ret = m_rpc_cli->init(s.IPs[j], port);
			if(ret >= 0){
				return ret;
			}

		}

		return -1;
	}

	int Dispatcher::reinitRPCClient(const StatusMap & m){
		
		if(!m.size()){
			return 0;
		}

		
		srand(time(NULL));
		int cnt = 0;
		while(cnt < 3){
			int idx = rand() % m.size();

			StatusMap::const_iterator itor = m.begin();
			for(int i = 0; itor != m.end() && i < idx; ++itor, ++i){
			}
			
			if(reinitRPCClient(itor->second) >= 0){
				return 0;
			}			
			++cnt;
		}

		return -1;
	}

	int Dispatcher::sendToClient(const string& cid, 
		const ServiceRequest& req){

		checkConnectServers();

		vector<CliSess> svs;
		int ret = getClientLoginServer(cid, svs);

		if(ret < 0){
			return ret;
		}
	
		ServiceRequest r = req;
		
		for(size_t i = 0; i < svs.size(); ++i){
			r.set_to_sessid(svs[i].first);
			r.set_from_sessid(svs[i].second->getSessID());
			r.set_from_type(svs[i].second->getSvType());	
			r.set_to_type(-1);	
			ret = svs[i].second->sendServiceRequest(r);
		}

		return ret;
	}

	int Dispatcher::sendToClient(const string& cid, 
		const ServiceResponse& resp){

		checkConnectServers();

		vector<CliSess> svs;
		int ret = getClientLoginServer(cid, svs);

		if(ret < 0){
			return ret;
		}

		ServiceResponse r = resp;
		
		for(size_t i = 0; i < svs.size(); ++i){
			r.set_to_sessid(svs[i].first);
			r.set_from_sessid(svs[i].second->getSessID());
			r.set_from_type(svs[i].second->getSvType());	
			r.set_to_type(-1);	
			ret = svs[i].second->sendServiceResponse(r);
		}

		return ret;

	}

	int Dispatcher::callService(const ServiceRequest& req, ServiceResponse& resp, 
		int  timeout_ms){
		if(m_rpc_cli)
			return m_rpc_cli->callService(req, resp, timeout_ms);

		return INNER_ERROR;
	}
};
