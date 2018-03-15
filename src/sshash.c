#include "sshash.h"

sshash_table* findPort(int key, sshash_table* table)
{
	sshash_table* s;
	HASH_FIND_INT(table,&key,s);
	return s;
}

int addPort(int key, sshash_ctx ctx, sshash_table* table)
{
	sshash_table* s = NULL;
	
	//first find if key existed
	s = findPort(key,table);
	if(s==NULL)
	{
		s = (sshash_table*)malloc(sizeof(sshash_table));
		if(s==NULL)
			return SS_ERR;
		s->key = key;
		s->ctx.dataUsage = ctx.dataUsage;
		s->ctx.time = ctx.time;

		HASH_ADD_INT(table,key,s);
		return SS_OK;
	}
	else
		return SS_ERR;
}

int deletePort(int key, sshash_table* table)
{
	sshash_table* s = NULL;
	s = findPort(key,table);
	if(s!=NULL)
	{
		HASH_DEL(table,s);
		free(s);
		return SS_OK;
	}
	else
		return SS_ERR;
}

void cleanPort(sshash_table* table)
{
	sshash_table* s = NULL;
	sshash_table* tmp = NULL;
	HASH_ITER(hh,table,s,tmp)
	{
		HASH_DEL(table,s);
		free(s);
	}
}

int countPort(sshash_table* table)
{
	return HASH_COUNT(table);
}
