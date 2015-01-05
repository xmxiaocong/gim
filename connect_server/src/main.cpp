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
#include "svlist_cache.h"
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

	//register servlist
	SvLstChFactory svcf;
	SvLstCache* svc = svcf.getSvLstCache(pSettings->SvLstCacheConfig);

	if(pSettings->Daemon){
		std::cout << "Daemon!\n";
		daemonize();
	}

	Serv sv;
	sv.id = pSettings->Id;
	sv.type = 0; 
	sv.v["ClientListenPort"] = pSettings->ClientListenPort;
	sv.v["ServerListenPort"] = pSettings->ServerListenPort;
	sv.v["ClientCount"] = 0;
	if (svc->addServ(pSettings->Type, sv) < 0) {
		std::cout << "zookeeper add_serv fail!\n";
		exit(4);
	}
	

	initNetLog(pSettings->NLogLevel, pSettings->NLogPath, 3600, true);
	logInit(pSettings->LogConfig);
	initStatistic(output_statistic);
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
		svc->updateServ(0, sv);
		sleep(1);
	}

	s.stopListen(pSettings->ServerListenPort);
	s.stopListen(pSettings->ClientListenPort);
	s.stop();
	svc->deleteServ(0, sv.id);

	return 0;
}
