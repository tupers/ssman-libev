#include "utils.h"

FILE* __g_logfile = NULL;

//socket create function;
int createUnixSocket(const char* path)
{
	unlink(path);
	
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(fd<0)
		return SS_ERR;
	
	struct sockaddr_un addr;
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);

	int len = offsetof(struct sockaddr_un, sun_path) + strlen(path);
	if(bind(fd,(struct sockaddr*)&addr, len)<0)
	{
		close(fd);
		return SS_ERR;
	}

	return fd;
}

int createUdpSocket(int port)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd<0)
		return SS_ERR;

	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if(bind(fd,(struct sockaddr*)&addr, sizeof(addr))<0)
	{
		close(fd);
		return SS_ERR;
	}

	return fd;
}
