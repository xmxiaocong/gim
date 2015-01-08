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

typedef string (*ConstructPack)(const string& sessid, int svtype, const string& sn, int status,
	const string& payload);

class Dispatcher{
public:
	Dispatcher():
		m_sesscache(NULL){
	}

	int init(const Json::Value& v){
		SsChFactory shf;
		m_sesscache = shf.getSessCache(v);
		
		if(!m_sesscache){
			return -1;
		}

		return 0;
	}

	~Dispatcher(){
		delete m_sesscache;
	}

	int addConnectServer(int svid, EventLoop* l, int conid);
	int delConnectServer(int svid);
	int getConnectServer(int svid, EventLoop*& l, int& conid);

	int sendServiceRequest(EventLoop* l, const string& cid, int svtype,
		const string& sn, const string& payload); 
	int sendServiceResponse(EventLoop* l, const string& cid, int svtype,
		const string& sn, int status, const string& payload); 
private:
	
	int sendServicePacket(EventLoop* l, const string& cid, int service_type, 
		const string& sn, int status, const string& payload, ConstructPack cp);

	int constructRequest(const string& sessid,
		int service_type,
		const string& sn,
		const string& payload,
		string& pack);
	struct con_srv_t{
		con_srv_t()
			:l(NULL),conid(0){
		}

		EventLoop* l;
		int conid;
	};
	map<int, con_srv_t> m_con_srv_map;	
	string m_last_error;
	SessCache* m_sesscache;
};


class DispatcherPool{
public:
	static Dispatcher* getDispatcher(const Json::Value& conf);
private:
};

};

#endif
