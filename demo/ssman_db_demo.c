#include "ssman_db.h"

int main()
{
	int ret = ssman_db_updateDb("/home/tupers/ssman-libev/ip.json","/home/tupers/ssman-libev/ssman_db_group.json","/tmp/test.db");
	if(ret == SS_ERR)
	{
		printf("Update db failed.");
		return -1;
	}

	ssman_db_obj obj;
	memset(&obj,0,sizeof(ssman_db_obj));

	ssman_db_config* cfg = ssman_db_loadConfig("/home/tupers/ssman-libev/ssman_db_conf.json");
	if(cfg == NULL)
	{
		printf("Failed to load config with path: \'%s\'.\n","/home/tupers/ssman-libev/ssman_db_conf.json");
		return -1;
	}
	obj.config = cfg;

	_LOG("start init...");
	if(ssman_db_init(&obj) == SS_ERR)
	{
		_LOG("init failed.");
		ssman_db_deinit(&obj);
		return -1;
	}

	ssman_db_deploy(&obj);
	ssman_db_exec(&obj);
	_LOG("main loop finished.");
	ssman_db_deinit(&obj);

	return 0;
}
