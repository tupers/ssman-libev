
typedef struct
{
	int size;
	int start;
	int cursor;
	char* map;
}pool_obj;

pool_obj* poolCreate(int size, int start);
void poolFree(pool_obj* obj);
int poolCursor(pool_obj* obj);
int poolSet(pool_obj* obj, int index, int val);
int poolCheck(pool_obj* obj, int index);
