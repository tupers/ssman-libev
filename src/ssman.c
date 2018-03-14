#include "ssman.h"

//static variable

//socket create function;
static int createUnixSocket(const char* path)
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

static int createUdpSocket(int port)
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

//ssman evio cb
static void ss_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		//log error
		return;
	}

	//recv data
	char buffer[SS_RECVBUF_SIZE];
	struct sockaddr_un remoteAddr;

	socklen_t remoteAddr_len = sizeof(struct sockaddr_un);
	int len = recvfrom(watcher->fd,buffer,sizeof(buffer),0,(struct sockaddr*)&remoteAddr,(socklen_t*)&remoteAddr_len);
	if(len<0)
	{
		//log error
		return;
	}
	buffer[len]='\0';

	//parse data drop wrong data
	//save data usage in hash table
	printf("from ss:%s\n",buffer);
}
static void web_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		//log error
		return;
	}

	//recv data
	char buffer[SS_RECVBUF_SIZE];
	struct sockaddr_in remoteAddr;

	socklen_t remoteAddr_len = sizeof(remoteAddr);
	int len = recvfrom(watcher->fd, buffer,sizeof(buffer), 0, (struct sockaddr*)&remoteAddr, &remoteAddr_len);
	if(len < 0)
	{
		//log error
		return;
	}
	buffer[len]='\0';

	//parse data drop wrong data
	//operate remote cmd
	//answer remote anyway
	printf("from web:%s\n",buffer);
}

int ssman_init(ssman_event* obj)
{
	//fill struc_event
	memset(obj,0,sizeof(ssman_event));
	obj->loop = EV_DEFAULT;
	obj->ioObjNum = SS_IOEVENT_NUM;
	obj->ioObj = (ssman_ioEvent*)malloc(sizeof(ssman_ioEvent)*obj->ioObjNum);
	if(obj->ioObj == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(obj->ioObj,0,sizeof(ssman_ioEvent)*obj->ioObjNum);

	//ss event with unix socket
	ssman_ioEvent* ssEvent = &(obj->ioObj[0]);
	ssEvent->fd = createUnixSocket(SS_UNIX_PATH);
	if(ssEvent->fd < 0)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	ssEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(ssEvent->watcher == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	ssEvent->cb = ss_cb;
	ev_io_init(ssEvent->watcher, ssEvent->cb, ssEvent->fd, EV_READ);

	//web event with udp socket
	ssman_ioEvent* webEvent = &(obj->ioObj[1]);
	webEvent->fd = createUdpSocket(SS_DEFAULT_PORT);
	if(webEvent->fd < 0)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	webEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(webEvent->watcher == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	webEvent->cb = web_cb;
	ev_io_init(webEvent->watcher, webEvent->cb, webEvent->fd, EV_READ);
	
	//bind event with loop
	ev_io_start(obj->loop, ssEvent->watcher);
	ev_io_start(obj->loop, webEvent->watcher);

	return SS_OK;
}

void ssman_deinit(ssman_event* obj)
{
	if(obj->ioObj == NULL)
		return;

	//free ssman_ioEvent
	int i;
	for( i=0; i<obj->ioObjNum; i++)
	{
		ssman_ioEvent* io = &(obj->ioObj[i]);
		if(io->fd>0)
			close(io->fd);
		if(io->watcher!=NULL)
			free(io->watcher);
	}

	//free ssman_event
	free(obj->ioObj);
}

void ssman_exec(ssman_event* obj)
{
	ev_run(obj->loop,0);
}

int main()
{
	ssman_event obj;
	if(ssman_init(&obj)==SS_OK)
	{
		ssman_exec(&obj);
		ssman_deinit(&obj);
	}
	return 0;
}
