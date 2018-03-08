#include "ssman.h"

ssman::ssman()
{
	
}

int ssman::createUnixSocket(string path)
{
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(fd<0)
		return -1;
	
	struct sockaddr_un addr;
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path.data());
	int len = offsetof(struct sockaddr_un, sun_path) + path.size();

	if(bind(fd,(struct sockaddr*)&addr, len)<0)
	{
		close(fd);
		return -1;
	}

	return fd;
}

int ssman::createUdpSocket(int port)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd<0)
		return -1;
	
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(fd,(struct sockaddr*)&addr, sizeof(addr))<0)
	{
		close(fd);
		return -1;
	}

	return fd;
}

int main()
{
#define ADDR "/home/tupers/test.sock"
	string path=ADDR;
	
	cout<<path.length()<<strlen(ADDR)<<endl;
}
