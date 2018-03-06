#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int init()
{
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("socket");
		return -1;

	if(bind(sockfd,(struct sockaddr*)&addr,sizeof(addr)) == -1)
	{
		perror("bind");
		close(sockfd);
		return -1;
	}
	
	if(listen(sockfd,5)==-1)
	{
		perror("error");
		close(sockfd);
		return -1;
	}

		

}
