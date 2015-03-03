#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <map>
#include "sess_cache.h"


namespace ef{
	class EventLoop;
}

namespace gim{

using namespace std;
using namespace ef;

enum{
	CID_OFFLINE = -20001,
	CONNECT_SERVER_OFFLINE = -20101,
	SEND_FAIL = -20201,	
};

class Dispatcher{
public:
	Dispatcher(EventLoop* l):
		m_evlp(l), m_sesscache(NULL){
	}

	int init(SsChFactory* shf){
		m_sesscache = shf->newSessCache();
		
		if(!m_sesscache){
			return -1;
		}

		return 0;
	}

	~Dispatcher(){
		delete m_sesscache;
	}

	int addConnectServer(int svid, int conid);
	int delConnectServer(int svid);
	int getConnectServer(int svid, int& conid);

	int sendServiceRequestToClient(const string& cid, int svtype,
		const string& sn, const string& payload, 
		const string& callback); 
	int sendServiceResponseToClient(const string& cid, int svtype,
		const string& sn, int status, const string& payload,
		const string& callback); 
private:
	
	typedef string (*ConstructPack)(const string& sessid, 
		int svtype, const string& sn, int status,
		const string& payload, const string& callback);

	int sendServicePacket(const string& cid, int service_type, 
		const string& sn, int status, const string& payload, 
		const string& callback, ConstructPack cp);

	int constructRequest(const string& sessid,
		int service_type,
		const string& sn,
		const string& payload,
		const string& callback,
		string& pack);

	map<int, int> m_con_srv_map;	
	EventLoop* m_evlp;
	SessCache* m_sesscache;
};


};

#endif
