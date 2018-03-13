#include <sqlite3.h>
#include <iostream>

using namespace std;

//sql cmd

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
