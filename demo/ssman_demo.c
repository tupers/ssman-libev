#include "ssman.h"

static void help()
{
	printf("\n");
	printf("\t----Help Information----\n");
	printf("\t-c\tconfig file path.\n");
	printf("\t[-f]\trun application in the background and pid file path .\n");
	printf("\t[-l]\tlog file path.\n");
	printf("\t[-h]\thelp information.\n");
}

int main(int argc, char **argv)
{
	int c;
	char* cfgPath 	= NULL;
	char* pidPath	= NULL;
	char* logPath 	= NULL;

	opterr = 0;

	while(( c = getopt(argc, argv, "c:f:hl:")) != -1)
		switch(c){
			case 'c':
				cfgPath = optarg;
				break;
			case 'f':
				pidPath = optarg;
				break;
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			case 'l':
				logPath = optarg;
				break;
			case '?':
				opterr = 1;
				break;
		}
	
	//check getopt error and cfgPath must be given.
	if(opterr || cfgPath == NULL){
		help();
		exit(EXIT_FAILURE);
	}
				
	//ssman_daemonize("/tmp/ssdaemon.pid");
	ssman_obj obj;
	memset(&obj,0,sizeof(ssman_obj));

	//load config
	ssman_config* cfg = ssman_loadConfig(cfgPath);
	if(cfg == NULL)
	{
		printf("Failed to load config with path: \'%s\'\n",cfgPath);
		exit(EXIT_FAILURE);
	}
	obj.config = cfg;

	//start log
	if(logPath)
		_LOG_FILE = fopen(logPath,"w");
	
	if(pidPath)
	{
		if(_LOG_FILE)
			printf("Start daemonize...,log will be logged in %s.\n",logPath);
		else
			printf("Start daemonize without log...\n");
		daemonize(pidPath);
	}

	//ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGABRT, SIG_IGN);

	_LOG("start init.");
	if(ssman_init(&obj)!=SS_OK)
	{
		_LOG("init failed.");
		_LOG_CLOSE;
		ssman_deinit(&obj);
		exit(EXIT_FAILURE);
	}

	ssman_exec(obj.event);
	_LOG("main loop finished.");
	ssman_deinit(&obj);
	_LOG_CLOSE;	

	exit(EXIT_SUCCESS);
}
