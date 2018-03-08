#include <ev++.h>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <string>
#include <iostream>
#include <stddef.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

class ssman
{
	public:
		ssman();
		~ssman();
	private:
		int createUnixSocket(string path);
		int createUdpSocket(int port);
		vector<ev::io*> m_vecWatcher;
		ev::default_loop* m_loop;
		int m_fdWebSock;
		int m_fdSSSock;
};
