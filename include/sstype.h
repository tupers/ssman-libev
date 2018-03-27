#ifndef _SSTYPE_H
#define _SSTYPE_H 

#include <ev.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//err code
#define SS_OK 	0
#define SS_ERR	-1

//ack
#define SS_ACK_OK 	"ok"
#define SS_ACK_ERR 	"err"

//default setting
#define SS_DB_IOEVENT_NUM	2
#define SS_DB_SSMAN_UDP_PORT	9005
#define SS_DB_WEB_UDP_PORT	9006

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
#endif /* ifndef _SSTYPE_H */
