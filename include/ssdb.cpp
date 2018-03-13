#include "ssdb.h"



ssdb::ssdb()
{

}

ssdb::~ssdb()
{

}

int ssdb::connect()
{
	int ret = sqlite3_open("/home/tupers/test.db",&m_db);
	if(ret)
	{
		cout<<"open database err."<<endl;
		return -1;
	}

	return 0;
}

int ssdb::disconnect()
{
	sqlite3_close(m_db);
	return 0;
}
