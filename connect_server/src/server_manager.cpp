#include "server_manager.h"
#include "proto/err_no.h"
#include "net/ef_event_loop.h"
#include "base/ef_md5.h"
#include "server_conn.h"
#include <cassert>
#include <iostream>
#include <algorithm>

namespace gim{

static ServMan g_servman;

static int32 hash_value(const std::string& k){
	uint8 m[16] = {0};
	MD5(m, (uint8*)k.data(), k.size());
	return *(int32*)m;
}

ServMan& getServMan(){
	return g_servman;
}

ServList::ServList():m_req_cnt(0){
	ef::mutexInit(&m_cs);
}

ServList::~ServList(){
	ef::mutexDestroy(&m_cs);
}

int32	ServList::addServer(int32 id, SrvCon* con, ef::EventLoop* l){
	ServerNode n(l, con, id);
	ef::AutoLock lk(&m_cs);
	m_nodes.push_back(n);
	std::sort(m_nodes.begin(), m_nodes.end());
	return 0;
}

int32	ServList::delServer(int32 id, SrvCon* con, ef::EventLoop* l){
	ServerNode n(l, con, id);
	NodeCmp c(l, con);
	ef::AutoLock lk(&m_cs); 
	Nodes::iterator it = remove_if(m_nodes.begin(), m_nodes.end(), c);
	m_nodes.erase(it, m_nodes.end());
	return 0;
}

int32	ServList::dispatchRequest(const std::string& key,
		const std::string& req, ef::EventLoop* const req_loop){
	int32 sendquesize = 0;
	int32 conid = 0;
	ServerNode n;
	{
		ef::AutoLock lk(&m_cs);
		int32 s = m_nodes.size();
		if(!s){
			return THIS_SERVICE_EMPTY;			
		}
		int32 idx = m_req_cnt;
		if(key.size()){
			idx = hash_value(key);
		}
		n = m_nodes[idx++ % s]; 
		assert(n.l);
		assert(n.con);
		sendquesize = n.con->sendQueueSize();
		conid = n.con->getId();
		++m_req_cnt;
	}

	if(n.con->sendQueueSize() > MAX_SEND_QUE_SIZE){
		std::cout << "conid:" << conid << ", noti:"
			<< n.con->getNotify() << ", sendQueueSize:"
			<< n.con->sendQueueSize() << std::endl;
			
		return SERVICE_TOO_BUSY;
	}
	if(req_loop != n.l)
		return (n.l)->asynSendMessage(conid, req);
	return (n.l)->sendMessage(conid, req);

}


ServMan::ServMan(){
	ef::mutexInit(&m_cs);
}

ServMan::~ServMan(){
	ef::mutexDestroy(&m_cs);
	ServMap::iterator it = m_server_maps.begin();
	for(; it != m_server_maps.end(); ++it){
		delete it->second;
	}	
}

ServList* ServMan::getAddServList(int32 type){
	ServList* l = NULL;
	ef::AutoLock lk(&m_cs);	
	ServMap::iterator it = m_server_maps.find(type);
	if(it != m_server_maps.end())
		return it->second;
	else{
		l = new ServList();
		m_server_maps[type] = l;
	}
	return l;
}

ServList* ServMan::getServList(int32 type){
	ServList* l = NULL;
	ef::AutoLock lk(&m_cs);	
	ServMap::iterator it = m_server_maps.find(type);
	if(it != m_server_maps.end()){
		return it->second;
	}
	return l;
}

int32 ServMan::addServer(int32 type, int32 svid, SrvCon* con, ef::EventLoop* l){
	ServList* lst = getAddServList(type);
	return lst->addServer(svid, con, l);
}

int32 ServMan::delServer(int32 type, int32 svid, SrvCon* con, ef::EventLoop* l){
	ServList* lst = getServList(type);
	if(lst){
		return lst->delServer(svid, con, l);
	}
	return NO_SERVICE; 
}

int32 ServMan::dispatchRequest(int32 type, const std::string& key, 
		const std::string& req, 
		ef::EventLoop* const req_loop){
	
	ServList* lst = getServList(type);
	if(lst){
		return lst->dispatchRequest(key, req, req_loop);
	}

	return NO_SERVICE; 
}



};
