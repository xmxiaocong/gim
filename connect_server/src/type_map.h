#ifndef __TYPE_MAP_H__
#define __TYPE_MAP_H__

#include <vector>
#include "base/ef_thread.h"

namespace gim{

class ServiceRequest;
class CliCon;


class SvList{
public:
	SvList();
	~SvList();

	int addServer(CliCon* c);
	int delServer(CliCon* c);
	

	int transRequest(const ServiceRequest& req);
private:
	ef::MUTEX m_cs;
	std::vector<CliCon*> m_svs;
	int m_cnt;
}; 

class TypeMap{
public:

	static int init(int maxtype = 1024);
	static int addServer(int type, CliCon* c);
	static int delServer(int type, CliCon* c);	

	static int transRequest(const ServiceRequest& req);
private:
	static std::vector<SvList> m_svlsts;
};

};

#endif/*__TYPE_MAP_H__*/
