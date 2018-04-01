#include "ssman_db.h"

static void help()
{
	printf("\n");
	printf("\t----Help Information----\n");
	printf("\t-c\tconfig file path.\n");
	printf("\t-u\tupdate data base.\n");
	printf("\t-d\tspecified data base path when set '-u'.\n");
	printf("\t-g\tspecified group json file path when set '-u'.\n");
	printf("\t-i\tspecified ip json file path when set '-u'.\n");
	printf("\t[-f]\trun application in the background with specified pid file path.\n");
	printf("\t[-l]\tlog file path.\n");
	printf("\t[-h]\thelp information.\n");
}

static char *_gets_s(char * str, int num)
{
	if (fgets(str, num, stdin) != 0)
	{
		size_t len = strlen(str);
		if (len > 0 && str[len-1] == '\n')
			str[len-1] = '\0';
		return str;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int c;
	char* ip_jsonPath	= NULL;
	char* group_jsonPath	= NULL;
	char* dbPath 		= NULL;
	char* cfgPath 		= NULL;
	char* pidPath		= NULL;
	char* logPath		= NULL;
	int update 		= 0;

	opterr = 0;
	
	while(( c = getopt(argc, argv, "c:d:f:g:hi:l:u")) != -1)
		switch(c){
			case 'c':
				cfgPath = optarg;
				break;
			case 'd':
				dbPath = optarg;
				break;
			case 'f':
				pidPath = optarg;
				break;
			case 'g':
				group_jsonPath = optarg;
				break;
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			case 'i':
				ip_jsonPath = optarg;
				break;
			case 'l':
				logPath = optarg;
				break;
			case 'u':
				update = 1;
				break;
			case '?':
				opterr = 1;
				break;
		}

	//there is two modes as below:
	//1) update database which need 'u' 'd' 'g' 'i'
	//2) daemon running in background need 'c', alternative 'f' 'l'
	if(opterr || (cfgPath == NULL && update == 0))
	{
		help();
		exit(EXIT_FAILURE);
	}
	
	//if 'u' be used run update db.
	if(update)
	{
		if(dbPath == NULL || ip_jsonPath == NULL || group_jsonPath == NULL)
			printf("Warning: Options in 'd' 'i' 'g' missing, 'u' update data base will not be continue.\n");
		else
			if(ssman_db_updateDb(ip_jsonPath,group_jsonPath,dbPath) == SS_ERR)
			{
				printf("Warning: Updating data base occur an error, this update operation may not work properly. \n");
				printf("Would you want to continue?(y/n)\n");
				char choice[SS_CFG_OPT_SIZE_SMALL];
				_gets_s(choice,SS_CFG_OPT_SIZE_SMALL-1);
				//gets(choice);
				if(strcmp(choice,"y") != 0)
					exit(EXIT_FAILURE);
			}
	}

	//start main process
	ssman_db_obj obj;
	memset(&obj,0,sizeof(ssman_db_obj));

	//load config
	ssman_db_config* cfg = ssman_db_loadConfig(cfgPath);
	if(cfg == NULL)
	{
		printf("Failed to load config with path: \'%s\'.\n",cfgPath);
		exit(EXIT_FAILURE);
	}
	obj.config = cfg;

	if(logPath)
		_LOG_FILE = fopen(logPath,"w");

	if(pidPath)
	{
		if(_LOG_FILE)
			printf("Start daemonize..., log will be logged in %s.\n",logPath);
		else
			printf("Start daemonize  without log...\n");
		daemonize(pidPath);
	}

	//ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGABRT, SIG_IGN);

	//add term signal cb
	//...
	
	//clean old
	//...
	
	_LOG("start init...");
	if(ssman_db_init(&obj) == SS_ERR)
	{
		_LOG("init failed.");
		_LOG_CLOSE;
		ssman_db_deinit(&obj);
		exit(EXIT_FAILURE);
	}

	if(ssman_db_deploy(&obj)!=SS_OK)
	{
		_LOG("deploy failed.");
		_LOG_CLOSE;
		ssman_db_deinit(&obj);
		exit(EXIT_FAILURE);
	}

	ssman_db_exec(&obj);
	_LOG("main loop finished.");

	//if finish gracefully, clean up
	//...
	//_LOG("clean up.");
	
	_LOG("free all resource.");
	ssman_db_deinit(&obj);
	_LOG("closed.");
	_LOG_CLOSE;

	exit(EXIT_SUCCESS);
}
