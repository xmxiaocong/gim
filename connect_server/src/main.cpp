#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include "net/ef_server.h"
#include "net/ef_net_log.h"
#include "base/ef_deamonize.h"
#include "base/ef_statistic.h"
#include "base/ef_utility.h"
#include "log_init.h"
#include "client_conn.h"
#include "server_conn.h"
#include "sess_cache.h"
#include "svlist_cache.h"
#include "user_db.h"
#include "settings.h"

using namespace ef;
using namespace gim;

Server* g_pdb = NULL;
int	g_run = 1;

int system_shutdown( void )
{
	g_run = false;

	return 0;
}


static void signal_handler(int sig)
{

	switch(sig) {
	case SIGHUP:
	case SIGTERM:
		system_shutdown();
		break;
	}

}


static int output_statistic(const std::string& l){
	logError("ConnectStatistic") << l;
	return 0;
}

Json::Value getJsonArray(const std::vector<std::string>& v){
	Json::Value a(Json::arrayValue);

	for(size_t i = 0; i < v.size(); ++i){
		a.append(v[i]);
	}

	return a;
}

int main(int argc, const char** argv){
	
	if(argc < 2){
		std::cout << "ConnectServer <config>\n";
		return 0;
	}

	Settings *pSettings = Singleton<Settings>::instance();

	if (!pSettings->load(argv[1])) {
		std::cout << "load setting file:" << argv[1]
			<< ", fail\n";
		exit(1);
	}	

	pSettings->print();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN); 
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT,  SIG_IGN);
	signal(SIGURG,  SIG_IGN);
	signal(SIGTERM, signal_handler);
	
	initNetLog(pSettings->NLogLevel, pSettings->NLogPath, 3600, true);
	logInit(pSettings->LogConfig);
	initStatistic(output_statistic);

	SsChFactory::init(pSettings->SessCacheConfig);
	UserDBFactory::init(pSettings->UserDBConfig);
	//register servlist
	SvLstChFactory::init(pSettings->SvLstCacheConfig);
	SvLstChFactory* svcf = SvLstChFactory::get();
	SvLstCache* svc = svcf->newSvLstCache();

	if(pSettings->Daemon){
		std::cout << "Daemon!\n";
		daemonize();
	}

	Serv sv;
	sv.id = pSettings->Id;
	sv.type = 0; 
	sv.v[GIM_PUBLIC_IPS] = getJsonArray(pSettings->PublicIPs);
	sv.v[GIM_LOCAL_IPS] = getJsonArray(pSettings->LocalIPs);
	sv.v[GIM_CLIENT_LISTEN_PORT] = pSettings->ClientListenPort;
	sv.v[GIM_SERVER_LISTEN_PORT] = pSettings->ServerListenPort;
	sv.v["ClientCount"] = 0;
	if (svc->addServer(pSettings->Type, sv) < 0) {
		std::cout << "add_serv fail!\n";
		exit(4);
	}
	

	Server s;
	s.setEventLoopCount(pSettings->ThreadCount);
	s.init();

	CliConFactory cfac(&s, pSettings->CliMaxIdleMs);
	CDispatcher cdisp(&s);

	if(s.startListen(pSettings->ClientListenPort, &cfac, &cdisp) < 0){
		std::cout << "listen client at:" << pSettings->ClientListenPort 
			<< " fail!" << std::endl;
		exit(1);
	}

	SrvConFactory sfac(&s, 0);
	if(s.startListen(pSettings->ServerListenPort, &sfac, 
		new SDispatcher(&s)) < 0){
		std::cout << "listen server at:" << pSettings->ServerListenPort 
			<< " fail!" << std::endl;
		exit(1);
	}
	s.run();

	while(g_run){
		sv.v["ClientCount"] = CliCon::totalCount();
		sv.v["ReportTimeStamp"] = int32(time(NULL));
		svc->updateServer(0, sv);
		sleep(1);
	}

	s.stopListen(pSettings->ServerListenPort);
	s.stopListen(pSettings->ClientListenPort);
	s.stop();
	svc->deleteServer(0, sv.id);
	delete svc;
	SvLstChFactory::free();
	UserDBFactory::free();
	SsChFactory::free();
	return 0;
}
