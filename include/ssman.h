#ifndef _SSMAN_H
#define _SSMAN_H value

#include "json.h"
#include "sshash.h"
#include "utils.h"

//event config
#define SS_IOEVENT_NUM 	2	//0:unix ss event; 1:udp web event;
#define SS_TOEVENT_NUM 	1	//0:send datausage from hash tabel to remote db
#define SS_TIMEOUT 	10
#define SS_DEFAULT_PORT 9000
#define SS_PULSE_LOCALPORT	9001
#define SS_PULSE_SERVERADDR	"127.0.0.1"
#define SS_PULSE_SERVERPORT	9002
#define SS_RECVBUF_SIZE 1024
#define SS_UNIX_PATH 	"/tmp/evtest.sock"

#define SS_CMD_SIZE	SS_RECVBUF_SIZE
#define SS_PIDFILE_SIZE	SS_CMD_SIZE/8
#define SS_CFG_SIZE	SS_CMD_SIZE*2
#define SS_RESULT_SIZE	SS_RECVBUF_SIZE
#define SS_CFG_OPT_SIZE	SS_CFG_SIZE/16


typedef struct{
	char manager_address[SS_CFG_OPT_SIZE];
	char method[SS_CFG_OPT_SIZE];
	char pulse_address[SS_CFG_OPT_SIZE];
	int web_port;
	int pulse_localPort;
	int pulse_serverPort;
	int pulseFd;//periodic sending data to remote database
	struct sockaddr_in remoteAddr;//target address structure that 'pulseFd' send to
}ssman_config;

typedef struct{
	//char cmd[16];
	int server_port;
	char password[32];
	ssman_config* config;
}ssman_cmd_detail;

typedef struct{
	ssman_event* event;
	ssman_config* config;
	sshash_table* portTable;
}ssman_obj;
//function
ssman_config* ssman_loadConfig(char* cfgPath);
int ssman_init(ssman_obj*);
void ssman_deinit(ssman_obj*);
void ssman_exec(ssman_event*);
int ssman_parseMsg_ss(char* msg, ssman_obj* obj);
int ssman_parseMsg_web(char* msg, ssman_obj* obj, char* result);
void ssman_deamonize(char* path);
#endif /* ifndef _SSMAN_H */
