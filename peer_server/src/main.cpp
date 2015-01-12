#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include "log_init.h"
#include "logic_server.h"
#include "peer_conn.h"
#include "push_conn.h"
#include "dispatcher.h"
#include "settings.h"
#include "net/ef_net_log.h"
#include "base/ef_deamonize.h"
#include "base/ef_statistic.h"
#include "base/ef_utility.h"

using namespace ef;
using namespace gim;

LogicServer* g_s = NULL;
int g_run = false;

int system_shutdown( void )
{
	g_run = true;
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
	logError("PeerStatistic") << l;
	return 0;
}


int main(int argc, const char** argv){
	
	if(argc < 2){
		std::cout << "ApnPushServer <config>\n";
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

	if(pSettings->Daemon){
		std::cout << "Daemon!\n";
		daemonize();
	}



	initNetLog(pSettings->NLogLevel, pSettings->NLogPath, 3600, true);
	logInit(pSettings->LogConfig);
	initStatistic(output_statistic);


	LogicServer s;

	g_s = &s;

	PushConFac* pcf = getPushConFactory();
	pcf->setLogicServer(&s);

	s.setSvConFactory(getPeerConFactory());
	s.setKeepAliveSpan(pSettings->KeepAliveSpan);
	s.setReconnectSpan(pSettings->ReconnectSpan);
	s.setServiceType(pSettings->ServiceType);
	s.init(pSettings->ThreadCount, pSettings->SvLstCacheConfig, 
		pSettings->SessCacheConfig);
	s.startListen(pSettings->PushListenPort, pcf);

	g_run = true;
	s.run();

	while(g_run){
		sleep(1);
	}

	s.stopListen(pSettings->PushListenPort);
	s.stop();

	return 0;
}
