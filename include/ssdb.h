#include <sqlite3.h>
#include <iostream>

using namespace std;

//sql cmd
#define SSDB_CREATE "CREATE TABLE managerDB("


class ssdb
{
	public:
		ssdb();
		int connect();	
		int disconnect();
		~ssdb();
	private:
		

		sqlite3 *m_db;
};
