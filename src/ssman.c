#include "ssman.h"

//static variable
static sshash_table* g_portTable = NULL;
static ssman_config* g_config = NULL;

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
static void ss_cb(EV_P_ ev_io* watcher, int revents)
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

	//parse data drop wrong data, save data usage in hash table
	int ret = ssman_parseMsg_ss(buffer);
	printf("from ss: msg: %s, result: %d\n",buffer,ret);
}

static void web_cb(EV_P_ ev_io* watcher, int revents)
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
	char* ack;

	int ret = ssman_parseMsg_web(buffer);
	if(ret == SS_OK)
		ack = SS_ACK_OK;
	else
		ack = SS_ACK_ERR;
	sendto(watcher->fd,ack,strlen(ack),0,(struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
	printf("from web: result: %d,msg: %s",ret,buffer);
}

static void sendtoDb_cb(EV_P_ ev_timer* watcher, int revents)
{
	//send all msg from hash table to remote db
	time_t timep;
	time(&timep);
	int num = countPort(&g_portTable);
	printf("time to send msg.%s",ctime(&timep));
	printf("hash table num: %d\n",num);
	//ev_break(EV_A_ EVBREAK_ALL);
	
	listPort(&g_portTable,NULL,&num);
	sshash_table** list = (sshash_table**)malloc(sizeof(sshash_table)*num);
	listPort(&g_portTable,list,NULL);
	int i;
	for(i=0;i<num;i++)
		printf("port: %d, data: %d\n",list[i]->key,list[i]->ctx.dataUsage);
	free(list);

}

//ssman utils
static char* createCmdString(ssman_cmd_detail* detail)
{
	static char cmd[SS_CMD_SIZE];
	memset(cmd,0,SS_CMD_SIZE);

	//build cmd
	snprintf(cmd,SS_CMD_SIZE,"ss-server -s 0.0.0.0 -l 1080 -p %d -k %s -m %s -u --manager-address %s -f /tmp/.shadowsocks_%d.pid",detail->server_port,detail->password,detail->config->method,detail->config->manager_address,detail->server_port);

	return cmd;
}

int ssman_loadConfig()
{
	g_config = (ssman_config*)malloc(sizeof(ssman_config));
	if(g_config == NULL)
	{
		//err log
		return SS_ERR;
	}
	
	strncpy(g_config->manager_address,SS_UNIX_PATH,32);
	g_config->manager_address[31]='\0';
	strncpy(g_config->method,"aes-256-cfb",16);
	g_config->method[15]='\0';

	return SS_OK;
}

int ssman_init(ssman_event* obj)
{
	if(obj == NULL)
		return SS_ERR;

	//fill event struct
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
	
	obj->toObjNum = SS_TOEVENT_NUM;
	obj->toObj = (ssman_toEvent*)malloc(sizeof(ssman_toEvent)*obj->toObjNum);
	if(obj->toObj == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(obj->toObj,0,sizeof(ssman_toEvent)*obj->toObjNum);

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
	
	// time out event with sending datausage to remote datebase
	ssman_toEvent* sendtoDbEvent = &(obj->toObj[0]);
	sendtoDbEvent->watcher = (ev_timer*)malloc(sizeof(ev_timer));
	if(sendtoDbEvent->watcher == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	sendtoDbEvent->cb = sendtoDb_cb;
	ev_timer_init(sendtoDbEvent->watcher, sendtoDbEvent->cb, SS_TIMEOUT, SS_TIMEOUT);

	//bind event with loop
	ev_io_start(obj->loop, ssEvent->watcher);
	ev_io_start(obj->loop, webEvent->watcher);
	ev_timer_start(obj->loop, sendtoDbEvent->watcher);

	return SS_OK;
}

void ssman_deinit(ssman_event* obj)
{
	int i;
	
	//free ssman_ioEvent
	if(obj->ioObj != NULL)
	{
		for( i=0; i<obj->ioObjNum; i++)
		{
			ssman_ioEvent* io = &(obj->ioObj[i]);
			if(io->fd>0)
				close(io->fd);
			if(io->watcher!=NULL)
				free(io->watcher);
		}
		
		free(obj->ioObj);
	}

	//free ssman_toEvent
	
	if(obj->toObj != NULL)
	{
		for( i=0; i<obj->toObjNum; i++)
		{
			ssman_toEvent* to = &(obj->toObj[i]);
			if(to->watcher!=NULL)
				free(to->watcher);
		}
		
		free(obj->toObj);
	}
	
}

void ssman_cleanGlobal()
{
	if(g_config)
	{
		free(g_config);
		g_config = NULL;
	}

	if(g_portTable)
	{
		cleanPort(&g_portTable);
		g_portTable = NULL;
	}

}


void ssman_exec(ssman_event* obj)
{
	ev_run(obj->loop,0);
}

int ssman_parseMsg_ss(char* msg)
{
	//ss-sever will send stat: {"0000":0}
	if(strncmp("stat",msg,4)!=0)
	{
		//wrong msg, drop and log
		return SS_ERR;
	}

	//now find the command ctx
	char* start = strchr(msg,'{');
	if(start == NULL)
	{
		//wrong command ctx, drop and log
		return SS_ERR;
	}
	
	json_value *obj = json_parse(start,strlen(msg)-(start-msg));
	if(obj==NULL)
	{
		//wrong json format, drop and log
		return SS_ERR;
	}
	
	if(obj->type != json_object)
	{
		//wrong json format, drop and log
		json_value_free(obj);
		return SS_ERR;
	}

	//take the first line json ctx
	char* name = obj->u.object.values[0].name;
	json_value* value = obj->u.object.values[0].value;
	if(value->type != json_integer)
	{
		//wrong json format, drop and log
		json_value_free(obj);
		return SS_ERR;
	}
	
	//add it into hash table
	sshash_ctx ctx;	
	time_t timep;

	time(&timep);
	ctx.dataUsage = value->u.integer;
	json_value_free(obj);

	return updatePort(atoi(name),ctx,&g_portTable);
}

int ssman_parseMsg_web(char* msg)
{
	//msg from web is a json file as show below
	//{
	//	"cmd"="xxx";
	//	"port"=1234;
	//	"password"="xxx";
	//}
	
	//position the ctx
	char* start =strchr(msg,'{');
	if(start == NULL)
	{
		//wrong command ctx, frop and log
		return SS_ERR;
	}

	json_value *obj = json_parse(start,strlen(msg)-(start-msg));
	if(obj == NULL || obj->type != json_object)
	{
		//wrong json format, drop and log
		return SS_ERR;
	}

	//init value for json
	ssman_cmd_detail detail;
	char cmd[16];
	memset(&detail,0,sizeof(ssman_cmd_detail));
	memset(cmd,0,16);
	detail.config = g_config;

	unsigned int i;
	for(i=0;i<obj->u.object.length;i++)
	{
		char* name = obj->u.object.values[i].name;
		json_value* value = obj->u.object.values[i].value;
		if(strcmp(name,"cmd")==0)
		{
			strncpy(cmd,value->u.string.ptr,16);
			cmd[15]='\0';
		}
		else if(strcmp(name,"server_port")==0)
			detail.server_port = value->u.integer;
		else if(strcmp(name,"password")==0)
		{
			strncpy(detail.password,value->u.string.ptr,32);
			detail.password[31]='\0';
		}
	}

	json_value_free(obj);

	char* systemCmd = NULL;
	//operate cmd
	if(strcmp(cmd,"add")==0)
	{
		//check neccesary option in detail
		if(strlen(detail.password) == 0 || detail.server_port == 0)
		{
			//err log
			return SS_ERR;
		}

		//build cmd for system()
		systemCmd = createCmdString(&detail);
		if(systemCmd == NULL)
		{
			//err log
			return SS_ERR;
		}

		printf("%s\n",systemCmd);
		//system()
		if(system(systemCmd) == -1)
		{
			//err log
			return SS_ERR;
		}

		//if system succeed, add this port into hash table
		sshash_ctx ctx;
		memset(&ctx,0,sizeof(sshash_ctx));
		if(addPort(detail.server_port,ctx,&g_portTable) == SS_ERR)
		{
			//err log
			return SS_ERR;
		}
	}

	return SS_OK;
}

int main()
{
	
	ssman_event obj;
	if(ssman_init(&obj)!=SS_OK)
	{
		printf("init failed.\n");
		ssman_deinit(&obj);
		return -1;
	}

	if(ssman_loadConfig()!=SS_OK)
	{
		printf("load config failed.\n");
		ssman_deinit(&obj);
		ssman_cleanGlobal();
		return -1;
	}
	
	ssman_exec(&obj);
	printf("finish.\n");
	ssman_deinit(&obj);
	ssman_cleanGlobal();
	
	return 0;
}
