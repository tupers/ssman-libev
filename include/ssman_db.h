#ifndef _SSMAN_DB_H
#define _SSMAN_DB_H value

#include <sqlite3.h>
#include "utils.h"

typedef ssman_event ssman_db_event;

#define SQL_CREATE_IPLIST 	"create table if not exists ipList(ip text primary key, ip_group integer default 0, flag integer default 0);"
#define SQL_CREATE_PORTLIST	"create table if not exists portList(port integer, password text default \'00000000\', ip_group integer default 0, dataUsage integer default 0, dataLimit integer default -1, strategy integer default 0, used integer default 0, owner integer default 0, flag integer default 0);"
#define SQL_LIST_TABLE		"select name from sqlite_master where type = 'table' order by name;"

typedef struct
{
	char* dbPath;
	int sendcmd_localPort;
	int sendcmd_serverPort;
	int sendcmd_fd;
	struct sockaddr_in ssmanAddr;
}ssman_db_config;

typedef struct
{
	ssman_db_event* event;
	ssman_db_config* config;
	sqlite3* db;
}ssman_db_obj;

typedef struct
{
	char passwd[SS_CFG_OPT_SIZE_SMALL];
	int port;
	int group;
	int used;
}_server_info;

typedef struct
{
	int i;
	_server_info* list;
}_server_list;

typedef struct
{
	int fd;
	int* plan_num;
	int* success_num;
	_server_info* info;
	struct sockaddr_in* addr;
}_server_sendtossman_cb_str;

int ssman_db_init(ssman_db_obj* obj);
void ssman_db_deinit(ssman_db_obj* obj);
void ssman_db_exec(ssman_db_obj* obj);
//ssman_db_config* ssman_db_loadConfig(char* cfgPath);
int ssman_db_updateDb(char* ipList, char* configPath, char* dbPath);
int ssman_db_start(ssman_db_obj* obj);

#endif /* ifndef _SSMAN_DB_H */
