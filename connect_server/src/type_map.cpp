#include "type_map.h"
#include <algorithm>
#include "client_conn.h"
#include "net/ef_event_loop.h"
#include "proto/msg_head.h"
#include "proto/err_no.h"
#include "proto/connect_server.pb.h"

namespace gim{

using namespace ef;
using namespace std;

SvList::SvList():m_cnt(0){
	mutexInit(&m_cs);		
}

SvList::~SvList(){
	mutexDestroy(&m_cs);
}

int SvList::addServer(CliCon* c){
	AutoLock l(&m_cs);
	m_svs.push_back(c);
	return 0;	
}

int SvList::delServer(CliCon* c){
	AutoLock l(&m_cs);
	vector<CliCon*>::iterator it = std::find(m_svs.begin(), m_svs.end(), c);	
	if(it != m_svs.end()){
		m_svs.erase(it);
	}
	return 0;
}

int SvList::transRequest(const ServiceRequest& req){

	if(!m_svs.size()){
		return NO_SERVICE; 
	}

	AutoLock l(&m_cs);

	CliCon* c = m_svs[m_cnt % m_svs.size()];

	if(!c){
		return NO_SERVICE;
	}

	string newreq;

	req.SerializeToString(&newreq);
	SendMsgOp *op = new SendMsgOp(c->getId(), SERVICE_REQ, newreq);
	int ret = c->getEventLoop()->addAsynOperator(op);
	if(ret < 0){
		return INNER_ERROR;
	}

	++m_cnt;
	if(m_cnt < 0){
		m_cnt = 0;
	}

	return 0;
}


vector<SvList> TypeMap::m_svlsts;

int TypeMap::init(int maxtype){
	m_svlsts.resize(maxtype);
	return 0;
}

int TypeMap::addServer(int type, CliCon* c){
	if(type <= 0 || type >= (int)m_svlsts.size()){
		return INVALID_TYPE;
	}
	
	int ret = m_svlsts[type-1].addServer(c);

	return ret;
}

int TypeMap::delServer(int type, CliCon* c){
	
	if(type <= 0 || type >= (int)m_svlsts.size()){
		return INVALID_TYPE;
	}

	int ret = m_svlsts[type-1].delServer(c);

	return ret;
}


int TypeMap::transRequest(const ServiceRequest& req){

	int type = req.to_type();

	if(type <= 0 || type >= (int)m_svlsts.size()){
		return INVALID_TYPE;
	}

	int ret = m_svlsts[type-1].transRequest(req);
	
	return ret;
}
	
};
