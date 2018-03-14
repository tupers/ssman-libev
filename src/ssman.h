#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>
#include <netinet/in.h>
#include <ev.h>

//event config
#define SS_IOEVENT_NUM 2	//0:unix ss event; 1:udp web event;
#define SS_DEFAULT_PORT 8000
#define SS_RECVBUF_SIZE 1024
#define SS_UNIX_PATH "/home/tupers/test.sock"

//err code
#define SS_OK	0
#define SS_ERR  -1

typedef void(*IO_CB)(struct ev_loop* loop, struct ev_io* watcher, int revents);

//ssman struct
typedef struct{
	int fd;
	ev_io* watcher;
	IO_CB cb;
}ssman_ioEvent;

typedef struct{
	struct ev_loop* loop;
	int ioObjNum;
	ssman_ioEvent* ioObj;
}ssman_event;

//function
int ssman_init(ssman_event*);
void ssman_deinit(ssman_event*);
void ssman_exec(ssman_event*);
