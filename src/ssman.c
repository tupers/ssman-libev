#include "ssman.h"

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
	int ret = ssman_parseMsg_ss(buffer,(ssman_obj*)watcher->data);
	//printf("from ss: msg: %s, result: %d\n",buffer,ret);
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

	int ret = ssman_parseMsg_web(buffer,(ssman_obj*)watcher->data);
	if(ret == SS_OK)
		ack = SS_ACK_OK;
	else
		ack = SS_ACK_ERR;
	sendto(watcher->fd,ack,strlen(ack),0,(struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
	//printf("from web: result: %d,msg: %s",ret,buffer);
}

static void sendtoDb_cb(EV_P_ ev_timer* watcher, int revents)
{
	sshash_table* table = ((ssman_obj*)watcher->data)->portTable;
	ssman_config* config = ((ssman_obj*)watcher->data)->config;

	char msg[] = "here";
	int len = sendto(config->pulseFd,msg,strlen(msg),0,(struct sockaddr*)&config->remoteAddr, sizeof(config->remoteAddr));
	//send all msg from hash table to remote db
	/*
	time_t timep;
	time(&timep);
	int num = countPort(&table);
	printf("time to send msg, length:%d .%s",len,ctime(&timep));
	printf("hash table num: %d\n",num);
	//ev_break(EV_A_ EVBREAK_ALL);
	
	listPort(&table,NULL,&num);
	sshash_table** list = (sshash_table**)malloc(sizeof(sshash_table)*num);
	listPort(&table,list,NULL);
	int i;
	for(i=0;i<num;i++)
		printf("port: %d, data: %d\n",list[i]->key,list[i]->ctx.dataUsage);
	free(list);
	*/

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

int ssman_loadConfig(ssman_config** conf)
{
	
	*conf = (ssman_config*)malloc(sizeof(ssman_config));
	if(*conf == NULL)
	{
		//err log
		return SS_ERR;
	}

	memset(*conf,0,sizeof(ssman_config));
	
	ssman_config* cfg = *conf;
	
	strncpy(cfg->manager_address,SS_UNIX_PATH,32);
	cfg->manager_address[31]='\0';
	strncpy(cfg->method,"aes-256-cfb",16);
	cfg->method[15]='\0';


	return SS_OK;
}

int ssman_init(ssman_obj* obj)
{
	if(obj == NULL)
		return SS_ERR;

	memset(obj,0,sizeof(ssman_obj));
	//-----------------load config--------------------------
	if(ssman_loadConfig(&(obj->config)) == SS_ERR)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	//init config
	ssman_config* config = obj->config;
	config->pulseFd = createUdpSocket(SS_PULSE_LOCALPORT);
	if(config->pulseFd < 0)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	config->remoteAddr.sin_family = AF_INET;
	config->remoteAddr.sin_port = htons(SS_PULSE_SERVERPORT);
	config->remoteAddr.sin_addr.s_addr = inet_addr(SS_PULSE_SERVERADDR);
	//-----------------create hashtabel--------------------------
	//...

	//-----------------create event--------------------------
	obj->event = (ssman_event*)malloc(sizeof(ssman_event));
	if(obj->event == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}

	//fill event struct
	ssman_event* event = obj->event;
	memset(event,0,sizeof(ssman_event));
	event->loop = EV_DEFAULT;
	event->ioObjNum = SS_IOEVENT_NUM;
	event->ioObj = (ssman_ioEvent*)malloc(sizeof(ssman_ioEvent)*event->ioObjNum);
	if(event->ioObj == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(event->ioObj,0,sizeof(ssman_ioEvent)*event->ioObjNum);
	
	event->toObjNum = SS_TOEVENT_NUM;
	event->toObj = (ssman_toEvent*)malloc(sizeof(ssman_toEvent)*event->toObjNum);
	if(event->toObj == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(event->toObj,0,sizeof(ssman_toEvent)*event->toObjNum);

	//ss event with unix socket
	ssman_ioEvent* ssEvent = &(event->ioObj[0]);
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
	ssEvent->watcher->data = obj;

	ssEvent->cb = ss_cb;
	ev_io_init(ssEvent->watcher, ssEvent->cb, ssEvent->fd, EV_READ);

	//web event with udp socket
	ssman_ioEvent* webEvent = &(event->ioObj[1]);
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
	webEvent->watcher->data = obj;

	webEvent->cb = web_cb;
	ev_io_init(webEvent->watcher, webEvent->cb, webEvent->fd, EV_READ);
	
	// time out event with sending datausage to remote datebase
	ssman_toEvent* pulseEvent = &(event->toObj[0]);
	pulseEvent->watcher = (ev_timer*)malloc(sizeof(ev_timer));
	if(pulseEvent->watcher == NULL)
	{
		ssman_deinit(obj);
		return SS_ERR;
	}
	pulseEvent->watcher->data = obj;

	pulseEvent->cb = sendtoDb_cb;
	ev_timer_init(pulseEvent->watcher, pulseEvent->cb, SS_TIMEOUT, SS_TIMEOUT);

	//bind event with loop
	ev_io_start(event->loop, ssEvent->watcher);
	ev_io_start(event->loop, webEvent->watcher);
	ev_timer_start(event->loop, pulseEvent->watcher);

	return SS_OK;
}

void ssman_deinit(ssman_obj* obj)
{
	int i;
	
	//first free ssman_event
	if(obj->event)
	{
		ssman_event* event = obj->event;
		//free ssman_ioEvent
		if(event->ioObj)
		{
			for( i=0; i<event->ioObjNum; i++)
			{
				ssman_ioEvent* io = &(event->ioObj[i]);
				if(io->fd>0)
					close(io->fd);
				if(io->watcher){
					ev_io_stop(event->loop,io->watcher);
					free(io->watcher);
				}
			}
			
			free(event->ioObj);
		}

		//free ssman_toEvent
		
		if(event->toObj)
		{
			for( i=0; i<event->toObjNum; i++)
			{
				ssman_toEvent* to = &(event->toObj[i]);
				if(to->watcher){
					ev_timer_stop(event->loop,to->watcher);
					free(to->watcher);
				}
			}
			
			free(event->toObj);
		}

		//free event loop
		//...
		
		free(obj->event);
	}

	//then free ssman_config
	if(obj->config)
	{
		ssman_config* config = obj->config;
		if(config->pulseFd)
			close(config->pulseFd);

		free(obj->config);
	}

	//free hash table
	if(obj->portTable)
		cleanPort(&(obj->portTable));

	//clean ssman_obj
	memset(obj,0,sizeof(ssman_obj));
}

void ssman_exec(ssman_event* obj)
{
	ev_run(obj->loop,0);
}

int ssman_parseMsg_ss(char* msg, ssman_obj* obj)
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
	
	json_value *json_obj = json_parse(start,strlen(msg)-(start-msg));
	if(obj==NULL)
	{
		//wrong json format, drop and log
		return SS_ERR;
	}
	
	if(json_obj->type != json_object)
	{
		//wrong json format, drop and log
		json_value_free(json_obj);
		return SS_ERR;
	}

	//take the first line json ctx
	char* name = json_obj->u.object.values[0].name;
	json_value* value = json_obj->u.object.values[0].value;
	if(value->type != json_integer)
	{
		//wrong json format, drop and log
		json_value_free(json_obj);
		return SS_ERR;
	}
	
	//add it into hash table
	sshash_ctx ctx;	
	time_t timep;

	time(&timep);
	ctx.dataUsage = value->u.integer;
	json_value_free(json_obj);

	return updatePort(atoi(name),ctx,&obj->portTable);
}

int ssman_parseMsg_web(char* msg, ssman_obj* obj)
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

	json_value *json_obj = json_parse(start,strlen(msg)-(start-msg));
	if(obj == NULL || json_obj->type != json_object)
	{
		//wrong json format, drop and log
		return SS_ERR;
	}

	//init value for json
	ssman_cmd_detail detail;
	char cmd[16];
	memset(&detail,0,sizeof(ssman_cmd_detail));
	memset(cmd,0,16);
	detail.config = obj->config;

	unsigned int i;
	for(i=0;i<json_obj->u.object.length;i++)
	{
		char* name = json_obj->u.object.values[i].name;
		json_value* value = json_obj->u.object.values[i].value;
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

	json_value_free(json_obj);

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

		//check hash table if the server existed
		sshash_table* s = findPort(detail.server_port, &obj->portTable);
		if(s)
			return SS_ERR;

		//build cmd for system()
		systemCmd = createCmdString(&detail);
		if(systemCmd == NULL)
		{
			//err log
			return SS_ERR;
		}

		//system()
		if(system(systemCmd) == -1)
		{
			//err log
			return SS_ERR;
		}

		//if system succeed, add this port into hash table
		sshash_ctx ctx;
		memset(&ctx,0,sizeof(sshash_ctx));
		addPort(detail.server_port,ctx,&obj->portTable);
	}
	else if(strcmp(cmd,"remove")==0)
	{
		//check neccesary option in detail
		if(detail.server_port == 0)
		{
			//err log
			return SS_ERR;
		}
		
		//check hash table if the server existed
		sshash_table* s = findPort(detail.server_port, &obj->portTable);
		if(s == NULL)
			return SS_ERR;

		//find specified ss-server pid file and kill server
		char path[64];
		snprintf(path,SS_CMD_SIZE,"/tmp/.shadowsocks_%d.pid",detail.server_port);
		FILE* pidFd = fopen(path,"r");
		if(pidFd == NULL)
			return SS_ERR;

		int pid;
		if(fscanf(pidFd,"%d",&pid) != EOF)
			kill(pid,SIGTERM);
		else
		{
			fclose(pidFd);
			return SS_ERR;
		}
		fclose(pidFd);

		//remove it from hash table
		deletePort(detail.server_port, &obj->portTable);
	}

	return SS_OK;
}

int main()
{
	
	ssman_obj obj;
	if(ssman_init(&obj)!=SS_OK)
	{
		printf("init failed.\n");
		ssman_deinit(&obj);
		return -1;
	}
	
	ssman_exec(obj.event);
	printf("finish.\n");
	ssman_deinit(&obj);
	
	return 0;
}
