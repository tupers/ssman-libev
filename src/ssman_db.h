#include <ev.h>
#include <sqlite3.h>
#include "sstype.h"

typedef struct
{
	char* dbPath;
}ssman_db_config;

typedef struct
{
	ssman_db_config* conifg;
	sqlite3* db;
}ssman_db_obj;

int ssman_db_init(ssman_db_obj* obj);
void ssman_db_deinit(ssman_db_obj* obj);
void ssman_db_exec(ssman_db_obj* obj);

