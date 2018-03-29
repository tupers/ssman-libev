#include "ssman_db.h"

int main()
{
	int ret = ssman_db_updateDb("/home/tupers/ssman-libev/ip.json","/home/tupers/ssman-libev/ssman_db_group.json","/tmp/test.db");	

	ssman_db_obj obj;
	memset(&obj,0,sizeof(ssman_db_obj));
	obj.config = (ssman_db_config*)malloc(sizeof(ssman_db_config));
	obj.config->dbPath = "/tmp/test.db";
	obj.config->sendcmd_localPort = 9004;
	obj.config->sendcmd_serverPort = 9000;
	ssman_db_init(&obj);
	ssman_db_start(&obj);
	ssman_db_exec(&obj);
	ssman_db_deinit(&obj);
	return 0;
}
