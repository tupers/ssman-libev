#ifndef _SSMAN_DB_H
#define _SSMAN_DB_H value

#include <sqlite3.h>
#include "utils.h"

typedef ssman_event ssman_db_event;

/*
 * used option:
 * 0:	not used,
 * 1:	running,
 * 2:	paused.when user datausage bigger than datalimit and will be refreshed in next peroid
 */
#define DB_PORT_NOLIMIT	0
#define DB_PORT_READY	0
#define DB_PORT_RUNNING	1
#define DB_PORT_PAUSED	2

#define SQL_CREATE_IPLIST 	"create table if not exists ipList(ip text primary key, ip_group integer default 0, flag integer default 0);"
#define SQL_CREATE_PORTLIST	"create table if not exists portList(port integer, password text default \'00000000\', ip_group integer default 0, dataUsage integer default 0, dataLimit integer default 0, strategy integer default 0, used integer default 0, flag integer default 0, peroid_start timestamp default current_date, peroid integer default 0, peroid_times integer default 0, owner integer default 0);"
#define SQL_LIST_TABLE		"select name from sqlite_master where type = 'table' order by name;"

#define SS_DB_DEFAULTPATH 	"/tmp/test.db"

typedef struct
{
	char db_path[SS_CFG_OPT_SIZE];
	int ssman_localPort;
	int ssman_serverPort;
	int ssman_pulsePort;
	int web_port;
	int ssman_fd;
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
	char password[SS_CFG_OPT_SIZE_SMALL];
	int port;
	int group;
	int dataUsage;
	int dataLimit;
	int strategy;
	int used;
}_server_info;

typedef struct
{
	int fd;
	struct sockaddr_in* addr;
}_server_ssman_udp;

typedef struct
{
	char cmd[SS_CFG_OPT_SIZE_SMALL];
	char password[SS_CFG_OPT_SIZE_SMALL];
	int plan_num;
	int success_num;
	_server_ssman_udp net;
}_server_ssman_cb_str;

typedef struct
{
	char ip[SS_IP_ADDR];
}_server_ip;

typedef struct
{
	int i;
	_server_ip* list;
}_server_ip_list;

int ssman_db_init(ssman_db_obj* obj);
void ssman_db_deinit(ssman_db_obj* obj);
void ssman_db_exec(ssman_db_obj* obj);
ssman_db_config* ssman_db_loadConfig(char* cfgPath);
int ssman_db_updateDb(char* ipList, char* configPath, char* dbPath);
int ssman_db_deploy(ssman_db_obj* obj);

#endif /* ifndef _SSMAN_DB_H */
