#include <stdio.h>
#include <netinet/in.h>
#include <ev.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT_NO 8000
#define BUFFER_SIZE 1024

void recv_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

int init(struct ev_loop* loop, struct ev_io* w_accept)
{
	int sockfd;
	struct sockaddr_in addr;

	//create server socket UDP
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
		perror("socket error");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NO);
	addr.sin_addr.s_addr = INADDR_ANY;

	//bind socket
	if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))!=0)
	{
		perror("bind error");
		close(sockfd);
		return -1;
	}

	//Initialize and start a watcher to receive udp connection
	ev_io_init(w_accept, recv_cb, sockfd, EV_READ);
	ev_io_start(loop,w_accept);

	return 0;
}

void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	char buffer[BUFFER_SIZE];
	struct sockaddr_in remoteAddr;

	if(EV_ERROR & revents)
	{
		perror("got invalid event");
		return;
	}

	socklen_t remoteAddr_len = sizeof(remoteAddr);

	int len = recvfrom(watcher->fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&remoteAddr, &remoteAddr_len);
	if(len == -1)
	{
		perror("recvfrom error");
		return;
	}
	buffer[len]=0;
	
		

	char msg[] = "ok\n";
	len = sendto(watcher->fd, msg, sizeof(msg), 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));

	if(len == -1)
	{
		perror("sendto error\n");
		return;
	}
	
	//memset(buffer,0,BUFFER_SIZE);	
}

int main()
{
	struct ev_loop* loop = ev_default_loop(0);
	struct ev_io w_accept;
	
	if(init(loop,&w_accept)!=0)
	{
		perror("init error");
		return -1;
	}
	
	while(1)
	{
		ev_loop(loop, 0);
	}

	return 0;
}	
