#ifndef _SSMAN_DB_H
#define _SSMAN_DB_H value

#include <sqlite3.h>
#include "utils.h"

typedef ssman_event ssman_db_event;

#define SQL_CREATE_IPLIST 	"create table if not exists ipList(ip text primary key, ip_group integer default 0);"
#define SQL_CREATE_PORTLIST	"create table if not exists portList(port integer, ip_group integer default 0, dataUsage integer default 0, dataLimit integer default -1, strategy integer default 0, used integer default 0);"
#define SQL_LIST_TABLE		"select name from sqlite_master where type = 'table' order by name;"

typedef struct
{
	char* dbPath;
}ssman_db_config;

typedef struct
{
	ssman_db_event* event;
	ssman_db_config* config;
	sqlite3* db;
}ssman_db_obj;

int ssman_db_init(ssman_db_obj* obj);
void ssman_db_deinit(ssman_db_obj* obj);
void ssman_db_exec(ssman_db_obj* obj);
int ssman_db_importIP(char* ipList, char* dbPath);
//ssman_db_config* ssman_db_loadConfig(char* cfgPath);

#endif /* ifndef _SSMAN_DB_H */
