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

#define UNIX_SOCK "/home/tupers/test.sock"
#define UDP_PORT 8000
#define BUFFER_SIZE 1024

class ssman
{
	public:
		ssman();
		~ssman();
		int init();
		void webCb(ev::io &w, int revents);
		void ssCb(ev::io &w, int revents);
		void exec();
	private:
		int createUnixSocket(string path);
		int createUdpSocket(int port);
		vector<ev::io*> m_vecWatcher;
		ev::default_loop m_loop;
		int m_fdWebSock=-1;
		int m_fdSSSock=-1;
};
