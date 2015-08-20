#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include <getopt.h>
#include "net/ef_sock.h"
#include "logic_server.h"
#include "test_conn.h"
#include "base/ef_deamonize.h"

gim::LogicServer* g_pdb = NULL;
int g_run = true;

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
			"-t	<type>			service type\n"
			"-p	<configpath>		config path\n"
			"-s	<statuspath>		status path\n"
			"-e	<connectstatuspath>	connect status path\n"
			"-z	<url>			zookeeper url\n"
			"-i	<id>			id\n"
			"-l	<logconfig>		log config file\n"
		<< std::endl;

}

int main(int argc, char* const* argv){

	const char* short_options = "hdt:p:s:e:z:i:l:";

	const struct option long_options[] = {
		{  "help",      0,   NULL,   'h'  },
		{  "daemon",      0,   NULL,   'd'  },
		{  "type",    1,   NULL,   't'  },
		{  "configpath",    1,   NULL,   'p'  },
		{  "statuspath",    1,   NULL,   's'  },
		{  "connectstatuspath",    1,   NULL,   'e'  },
		{  "zookeeper",   1,   NULL,   'z'  },
		{  "id",   1,   NULL,   'i'  },
		{  "logconfig",   1,   NULL,   'l'  },
		{  NULL,      0,    NULL,   0  }
	};

	int c;

	std::string type;
	std::string connectstatuspath;
	std::string configpath;
	std::string statuspath;
	std::string zkurl;
	std::string id;
	std::string logconfig;
	bool dm = false;

	while((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1){
		switch(c){
		case 'h':
			printHelpInfo();
			return 0;

		case 'd':
			dm = true;
			break;

		case 't':
			type = optarg;
			break;

		case 'p':
			configpath = optarg;
			break;		
		
		case 's':
			statuspath = optarg;
			break;

		case 'e':
			connectstatuspath = optarg;
			break;

		case 'z':
			zkurl = optarg;
			break;
		
		case 'i':
			id = optarg;
			break;

		case 'l':
			logconfig = optarg;
			break;	

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
	gim::TestConnFact tf;

	int ret = 0;
	gim::LogicServer s;
	g_pdb = &s;

	if(!type.size() || !id.size()){
		printHelpInfo();
		return -1;
	}



	s.setSvConFactory(&tf);

	ret = s.init(zkurl, configpath, statuspath, connectstatuspath, 
		atoi(type.data()), atoi(id.data()), logconfig);

	if(ret < 0){
		std::cout << "LogicServer init fail\n";
		return ret;
	}	

	s.start();

	while(g_run){
		std::cout << "running!\n";
		sleep(1);
	}

	s.stop();

	return 0;
}
