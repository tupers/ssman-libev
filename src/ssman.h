#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>
#include <netinet/in.h>
#include <time.h>
#include <ev.h>
#include "sstype.h"

//event config
#define SS_IOEVENT_NUM 	2	//0:unix ss event; 1:udp web event;
#define SS_TOEVENT_NUM 	1	//0:send datausage from hash tabel to remote db
#define SS_TIMEOUT 	10
#define SS_DEFAULT_PORT 8000
#define SS_RECVBUF_SIZE 1024
#define SS_UNIX_PATH 	"/home/tupers/test.sock"

typedef void(*IO_CB)(EV_P_ ev_io* watcher, int revents);
typedef void(*TO_CB)(EV_P_ ev_timer* watcher, int revents);

//ssman struct
typedef struct{
	int fd;
	ev_io* watcher;
	IO_CB cb;
}ssman_ioEvent;

typedef struct{
	ev_timer* watcher;
	TO_CB cb;
}ssman_toEvent;

typedef struct{
	struct ev_loop* loop;
	int ioObjNum;
	int toObjNum;
	ssman_ioEvent* ioObj;
	ssman_toEvent* toObj;
}ssman_event;

//function
int ssman_init(ssman_event*);
void ssman_deinit(ssman_event*);
void ssman_exec(ssman_event*);