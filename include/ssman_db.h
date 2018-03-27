#include <sqlite3.h>
#include "sstype.h"
#include "utils.h"

typedef ssman_event ssman_db_event;

#define SQL_CREATE_IPLIST 	"create table if not exists ipList(ip text primary key, group int default 0)"
#define SQL_CREATE_PORTLIST	"create table if not exists portList(port int, group int default 0, dataUsage int default 0, dataLimit int default -1, strategy int default 0, used int default 0)"

typedef struct
{
	char* dbPath;
}ssman_db_config;

typedef struct
{
	ssman_db_event* event;
	ssman_db_config* conifg;
	sqlite3* db;
}ssman_db_obj;

int ssman_db_init(ssman_db_obj* obj);
void ssman_db_deinit(ssman_db_obj* obj);
void ssman_db_exec(ssman_db_obj* obj);


