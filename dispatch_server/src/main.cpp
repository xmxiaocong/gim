#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include <getopt.h>
#include "dispatch_server.h"
#include "base/ef_deamonize.h"

gim::DispatchServer* g_pdb = NULL;

int system_shutdown( void )
{
	g_pdb->stop();

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


Json::Value getJsonArray(const std::vector<std::string>& v){
	Json::Value a(Json::arrayValue);

	for(size_t i = 0; i < v.size(); ++i){
		a.append(v[i]);
	}

	return a;
}

static void printHelpInfo(){
	std::cout << 	"-h				help\n"
			"-d				daemon\n"
			"-c	<file>			config file\n"
		<< std::endl;

}

int main(int argc, char* const* argv){
	
	const char* short_options = "hdc:";

	const struct option long_options[] = {
		{  "help",      0,   NULL,   'h'  },
		{  "daemon",      0,   NULL,   'd'  },
		{  "configfile",    1,   NULL,   'c'  },
		{  NULL,      0,    NULL,   0  }
	};

	int c;

	std::string configfile;
	bool dm = false;

	while((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1){
		switch(c){
		case 'h':
			printHelpInfo();
			return 0;

		case 'd':
			dm = true;
			break;

	/*	case 't':
			configtype = optarg;
			break;

		case 'p':
			configpath = optarg;
			break;		
		
		case 's':
			statuspath = optarg;
			break;
*/
		case 'c':
			configfile = optarg;
			break;
/*
		case 'z':
			zkurl = optarg;
			break;
		
		case 'i':
			id = optarg;
			break;

		case 'l':
			logconfig = optarg;
			break;	
*/
		}			
	}
	
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN); 
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT,  SIG_IGN);
	signal(SIGURG,  SIG_IGN);
	signal(SIGTERM, signal_handler);

	if(dm){
		ef::daemonize();
	}

	int ret = 0;
	gim::DispatchServer s;
	g_pdb = &s;

//	if(!configtype.size() || !id.size()){
//		printHelpInfo();
//		return -1;
//	}

	ret = s.init(configfile);
/*
	if(configfile.size()){
		ret = s.initByFile(configfile, atoi(id.data()), logconfig);
	}else{
		ret = s.initByZK(zkurl, configpath, statuspath, atoi(id.data()), logconfig);
	}
*/
	if(ret < 0){
		std::cout << "DispatchServer init fail\n";
		return ret;
	}	
#if 0
	SsChFactory::init(pSettings->SessCacheConfig);
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
	

	CliConFactory cfac(&s, pSettings->CliMaxIdleMs);
	CDispatcher cdisp(&s);

	if(s.startListen(pSettings->ClientListenPort, &cfac, &cdisp) < 0){
		std::cout << "listen client at:" << pSettings->ClientListenPort 
			<< " fail!" << std::endl;
		exit(1);
	}

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
#endif

	ret = s.start();
	
	if(ret < 0){
		std::cout << "DispatchServer start fail\n";
	}

	s.stop();

//	s.free();

	return 0;
}
