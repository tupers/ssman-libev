#include "pool.h"

pool_obj* poolCreate(int size, int start)
{
	if(size <= 0)
		return NULL;

	pool_obj* obj = (pool_obj*)malloc(sizeof(pool_obj));
	if(obj == NULL)
		return NULL;

	memset(obj,0,sizeof(pool_obj));

	obj->size = size;
	obj->start = start;
	obj->cursor = obj->start;
	obj->map = (char*)malloc(sizeof(char)*obj->size);
	if(obj->map == NULL)
	{
		free(obj);
		return NULL;
	}

	memset(obj->map,0,sizeof(char)*obj->size);
	
	return obj;
}

void poolFree(pool_obj* obj)
{
	if(obj->map)
		free(obj->map);

	if(obj)
		free(obj);
}

int poolCursor(pool_obj* obj)
{
	if(obj->cursor == obj->start -1)
		obj->cursor = obj->start;

	int i;
	for(i=obj->cursor;i<(obj->start+obj->size);i++)
		if(obj->map[i] == 0)
		{
			obj->cursor = i;
			return obj->cursor;
		}

	for(i=obj->start;i<obj->cursor;i++)
		if(obj->map[i] == 0)
		{
			obj->cursor = i;
			return obj->cursor;
		}

	obj->cursor = obj->start - 1;
	return obj->cursor;
}

int poolSet(pool_obj* obj, int index, int val)
{
	//check index
	if(poolCheck(obj,index)<0)
		return -1;

	obj->map[index] = val;
	return 0;
}

int poolCheck(pool_obj* obj, int index)
{
	if(index<obj->start || index>(obj->start + obj->size))
		return -1;
	else
		return obj->map[index];
}
