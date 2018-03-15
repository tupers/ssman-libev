#include <uthash.h>
#include <time.h>
#include "sstype.h" 

typedef struct{
	int dataUsage;
	time_t time;
}sshash_ctx;

typedef struct{
	int key;//port
	sshash_ctx ctx;
	UT_hash_handle hh;
}sshash_table;

int addPort(int key, sshash_ctx ctx, sshash_table* table);
sshash_table* findPort(int key, sshash_table* table);
int deletePort(int key, sshash_table* table);
void cleanPort(sshash_table* table);
int countPort(sshash_table* table);

