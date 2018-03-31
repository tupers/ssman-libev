#ifndef _SSHASH_H
#define _SSHASH_H value

#include <uthash.h>
#include "sstype.h" 

typedef struct{
	int lastUsage;
	int dataUsage;
}sshash_ctx;

typedef struct{
	int key;//port
	sshash_ctx ctx;
	UT_hash_handle hh;
}sshash_table;

int addPort(int port, sshash_ctx* ctx, sshash_table** table);
sshash_table* findPort(int key, sshash_table** table);
int deletePort(int key, sshash_table** table);
void cleanPort(sshash_table** table);
int countPort(sshash_table** table);
void listPort(sshash_table** table, sshash_table** list, int* listNum);
#endif /* ifndef _SSHASH_H */
