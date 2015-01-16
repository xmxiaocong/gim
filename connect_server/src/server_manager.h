#ifndef __SERVER_MANAGER_H__
#define __SERVER_MANAGER_H__

#include <map>
#include <string>
#include <vector>
#include "base/ef_thread.h"
#include "base/ef_btype.h"

namespace ef{
	class EventLoop;
};

namespace gim{

	enum{
		MAX_SEND_QUE_SIZE = 2048,
	};

	using namespace ef;

	class SrvCon;
	
	class ServList{
	public:	
		ServList();
		~ServList();
	
		int32 addServer(SrvCon* con, ef::EventLoop* l);
		int32 delServer(SrvCon* con, ef::EventLoop* l);	
		int32 dispatchRequest(const std::string& key, 
			const std::string& req, 
			ef::EventLoop* const req_loop);
	private:
		struct ServerNode{
			EventLoop* l;
			SrvCon* con;
			ServerNode(EventLoop* pl = NULL, SrvCon* con = 0):
				l(pl), con(con){
			}

			bool operator==(const ServerNode& n) const{
				return n.l == l && n.con == con;
			}
		};

		struct NodeCmp{
			NodeCmp(EventLoop* pl, SrvCon* con):
				cmpnd(pl, con){
			}

			bool operator()(const ServerNode& n) const{
				return cmpnd == n;
			}
			
			ServerNode cmpnd;
		};

		int32 m_req_cnt;
		typedef std::vector<ServerNode> Nodes;
		Nodes m_nodes; 	
		ef::MUTEX m_cs;
	};


	class ServMan{
	public:
		ServMan();
		~ServMan();

		int32 addServer(int32 type, SrvCon* con, ef::EventLoop* l);
		int32 delServer(int32 type, SrvCon* con, ef::EventLoop* l);	
		int32 dispatchRequest(int32 type, const std::string& key,
			const std::string& req, ef::EventLoop* const req_loop);
	private:
		ServList* getAddServList(int32 type);
		ServList* getServList(int32 type);

		typedef std::map<int32, ServList*> ServMap;
		ServMap m_server_maps;
		ef::MUTEX m_cs;
	};

	ServMan& getServMan();

}

#endif/*SERVER_MANAGER_H*/
