#include <stdio.h>
#include <netinet/in.h>
#include <ev.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>

#define PORT_NO 8000
#define BUFFER_SIZE 1024

void recv_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);
void urecv_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

int init(struct ev_loop* loop, struct ev_io* w_accept)
{
	int sockfd;
	struct sockaddr_in addr;
	struct sockaddr_un uaddr;
	int usockfd;

	//create server socket UDP
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<0)
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
	ev_io_init(&w_accept[0], recv_cb, sockfd, EV_READ);
	ev_io_start(loop,&w_accept[0]);
	
	#define SRC_ADDR "/home/tupers/test.sock"
	unlink(SRC_ADDR);

	usockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(usockfd<0)
	{
		perror("unix socket error");
		close(sockfd);
		return -1;
	}
	memset(&uaddr, 0, sizeof(uaddr));
	uaddr.sun_family = AF_UNIX;
	strcpy(uaddr.sun_path, SRC_ADDR);
	int len = offsetof(struct sockaddr_un, sun_path) + sizeof(SRC_ADDR);
	if(bind(usockfd,(struct sockaddr*)&uaddr, len)<0)
		perror("bind unix socket error");
	else
	{
		ev_io_init(&w_accept[1], urecv_cb, usockfd, EV_READ);
		ev_io_start(loop, &w_accept[1]);		
	}

	return 0;
}

void urecv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	char buffer[BUFFER_SIZE];
	struct sockaddr_un remoteAddr;

	if(EV_ERROR & revents)
	{
		perror("got invalid event");
		return;
	}

	socklen_t remoteAddr_len = sizeof(struct sockaddr_un);
	int len = recvfrom(watcher->fd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&remoteAddr,(socklen_t*)&remoteAddr_len);
	buffer[len]='\0';
	printf("%s\n",buffer);
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
	
}

int main()
{
	struct ev_loop* loop = ev_default_loop(0);
	struct ev_io w_accept[2];
		
	if(init(loop,w_accept)!=0)
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
