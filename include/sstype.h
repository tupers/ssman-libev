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

#define SSMAN_VERSION 0.3

//err code
#define SS_OK 	0
#define SS_ERR	-1

//ack
#define SS_ACK_OK 	"ok"
#define SS_ACK_ERR 	"err"
#define SS_JSON_SUCCESS	"\"ret\":\"success\""
#define SS_JSON_FAILED	"\"ret\":\"failed\""

//size
#define SS_RECVBUF_SIZE 1024
#define SS_CMD_SIZE	SS_RECVBUF_SIZE
#define SS_PIDFILE_SIZE	SS_CMD_SIZE/8
#define SS_CFG_SIZE	SS_CMD_SIZE*2
#define SS_RESULT_SIZE	SS_RECVBUF_SIZE
#define SS_CFG_OPT_SIZE	SS_CFG_SIZE/16
#define SS_CFG_OPT_SIZE_SMALL	SS_CFG_OPT_SIZE/8
#define SS_CFG_OPT_SIZE_LARGE	SS_CFG_OPT_SIZE*2
#define SS_IP_ADDR	SS_CFG_OPT_SIZE_SMALL

//default setting
#define SS_DB_IOEVENT_NUM	2
#define SS_DB_WEB_UDP_PORT	9003
#define SS_WEB_UDP_PORT 	9000
#define SS_PULSE_LOCALPORT	9001
#define SS_PULSE_SERVERADDR	"127.0.0.1"
#define SS_PULSE_SERVERPORT	9002
#define SS_DB_SSMAN_PULSEPORT	SS_PULSE_SERVERPORT
#define SS_DB_SSMAN_LOCALPORT	9004
#define SS_DB_SSMAN_SERVERPORT	SS_WEB_UDP_PORT

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
