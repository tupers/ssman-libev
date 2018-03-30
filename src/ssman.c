#include "ssman.h"

//ssman evio cb
static void ss_cb(EV_P_ ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		_LOG("ss cb error.");
		return;
	}

	//recv data
	char buffer[SS_RECVBUF_SIZE];
	struct sockaddr_un remoteAddr;

	socklen_t remoteAddr_len = sizeof(struct sockaddr_un);
	int len = recvfrom(watcher->fd,buffer,sizeof(buffer),0,(struct sockaddr*)&remoteAddr,(socklen_t*)&remoteAddr_len);
	if(len<0)
	{
		_LOG("ss cb recv error.");
		return;
	}
	buffer[len]='\0';

	//parse data drop wrong data, save data usage in hash table
	int ret = ssman_parseMsg_ss(buffer,(ssman_obj*)watcher->data);
	if(ret == SS_ERR)
		_LOG("ss cb parse msg error");
}

static void web_cb(EV_P_ ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		_LOG("web cb error.");
		return;
	}

	//recv data
	char buffer[SS_RECVBUF_SIZE];
	struct sockaddr_in remoteAddr;

	socklen_t remoteAddr_len = sizeof(remoteAddr);
	int len = recvfrom(watcher->fd, buffer,sizeof(buffer), 0, (struct sockaddr*)&remoteAddr, &remoteAddr_len);
	if(len < 0)
	{
		_LOG("web cb recv error.");
		return;
	}
	buffer[len]='\0';

	//parse data drop wrong data
	//operate remote cmd
	char result[SS_RESULT_SIZE];
	
	int ret = ssman_parseMsg_web(buffer,(ssman_obj*)watcher->data,result);
	if(ret == SS_ERR)
		_LOG("web cb parse msg error.");

	sendto(watcher->fd,result,strlen(result),0,(struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
}

static void sendtoDb_cb(EV_P_ ev_timer* watcher, int revents)
{
	//send all msg from hash table to remote db
	sshash_table* table = ((ssman_obj*)watcher->data)->portTable;
	ssman_config* config = ((ssman_obj*)watcher->data)->config;
	
	int num = countPort(&table);
	if(num)
	{
		//get port list from table
		listPort(&table,NULL,&num);
		sshash_table** list = (sshash_table**)malloc(sizeof(sshash_table)*num);
		if(list == NULL)
			return;
		listPort(&table,list,NULL);
		
		//build str head 
		char msg[SS_RESULT_SIZE];
		snprintf(msg,SS_RESULT_SIZE,"{");
		
		int i;
		for(i=0;i<num;i++)
		{
			int len = strlen(msg);
			snprintf(msg+len,SS_RESULT_SIZE-len,"\"%d\":%d,",list[i]->key,list[i]->ctx.dataUsage);
		}
		free(list);

		int len = strlen(msg);
		//drop last ',' and finish tail
		snprintf(msg+len-1,SS_RESULT_SIZE-len+1,"}");
		
		//send to remote data base
		sendto(config->pulseFd,msg,strlen(msg),0,(struct sockaddr*)&config->remoteAddr, sizeof(config->remoteAddr));
	}
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


ssman_config* ssman_loadConfig(char* cfgPath)
{
	if(cfgPath == NULL)
		return NULL;
	
	//get json obj
	json_value* json_obj = openJsonConfig(cfgPath);
	if(json_obj == NULL)
		return NULL;

	//malloc ssman config
	ssman_config* cfg = (ssman_config*)malloc(sizeof(ssman_config));
	if(cfg == NULL)
	{
		closeJsonConfig(json_obj);
		return NULL;
	}

	memset(cfg,0,sizeof(ssman_config));
	
	//fill ssman config	
	unsigned int i;
	for(i=0;i<json_obj->u.object.length;i++)
	{
		char* name = json_obj->u.object.values[i].name;
		json_value* value = json_obj->u.object.values[i].value;
		if(strcmp(name,"manager_address")==0)
		{
			strncpy(cfg->manager_address,value->u.string.ptr,SS_CFG_OPT_SIZE);
			cfg->manager_address[SS_CFG_OPT_SIZE-1]='\0';
		}
		else if(strcmp(name,"method")==0)
		{
			strncpy(cfg->method,value->u.string.ptr,SS_CFG_OPT_SIZE);
			cfg->method[SS_CFG_OPT_SIZE-1]='\0';
		}
		else if(strcmp(name,"pulse_address")==0)
		{
			strncpy(cfg->pulse_address,value->u.string.ptr,SS_CFG_OPT_SIZE);
			cfg->pulse_address[SS_CFG_OPT_SIZE-1]='\0';
		}
		else if(strcmp(name,"pulse_localPort")==0)
			cfg->pulse_localPort = value->u.integer;
		else if(strcmp(name,"pulse_serverPort")==0)
			cfg->pulse_serverPort = value->u.integer;
		else if(strcmp(name,"web_port")==0)
			cfg->web_port = value->u.integer;

	}
	closeJsonConfig(json_obj);

	//check ssman config if opt is NULL give it a default value
	if(cfg->manager_address[0] == '\0')
	{
		strncpy(cfg->manager_address,SS_UNIX_PATH,SS_CFG_OPT_SIZE);
		cfg->manager_address[SS_CFG_OPT_SIZE-1]='\0';
	}

	if(cfg->method[0] == '\0')
	{
		strncpy(cfg->method,"aes-256-cfb",SS_CFG_OPT_SIZE);
		cfg->method[SS_CFG_OPT_SIZE-1]='\0';
	}

	if(cfg->pulse_address[0] == '\0')
	{
		strncpy(cfg->pulse_address,SS_PULSE_SERVERADDR,SS_CFG_OPT_SIZE);
		cfg->pulse_address[SS_CFG_OPT_SIZE-1]='\0';
	}

	if(cfg->pulse_localPort == 0)
		cfg->pulse_localPort = SS_PULSE_LOCALPORT;

	if(cfg->pulse_serverPort == 0)
		cfg->pulse_serverPort = SS_PULSE_SERVERPORT;

	if(cfg->web_port == 0)
		cfg->web_port = SS_DEFAULT_PORT;

	return cfg;
}

int ssman_init(ssman_obj* obj)
{
	if(obj == NULL)
		return SS_ERR;

	//init config
	ssman_config* config = obj->config;
	config->pulseFd = createUdpSocket(config->pulse_localPort);
	if(config->pulseFd < 0)
	{
		_LOG("Create pulse udp socket failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}
	config->remoteAddr.sin_family = AF_INET;
	config->remoteAddr.sin_port = htons(config->pulse_serverPort);
	config->remoteAddr.sin_addr.s_addr = inet_addr(config->pulse_address);
	//-----------------create hashtabel--------------------------
	//...

	//-----------------create event--------------------------
	obj->event = (ssman_event*)malloc(sizeof(ssman_event));
	if(obj->event == NULL)
	{
		_LOG("Malloc ssman_event failed.");
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
		_LOG("Malloc io event failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(event->ioObj,0,sizeof(ssman_ioEvent)*event->ioObjNum);
	
	event->toObjNum = SS_TOEVENT_NUM;
	event->toObj = (ssman_toEvent*)malloc(sizeof(ssman_toEvent)*event->toObjNum);
	if(event->toObj == NULL)
	{
		_LOG("Malloc to event failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}
	memset(event->toObj,0,sizeof(ssman_toEvent)*event->toObjNum);

	//ss event with unix socket
	ssman_ioEvent* ssEvent = &(event->ioObj[0]);
	ssEvent->fd = createUnixSocket(config->manager_address);
	if(ssEvent->fd < 0)
	{
		_LOG("Create unix socket failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}

	ssEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(ssEvent->watcher == NULL)
	{
		_LOG("Malloc ev_io failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}
	ssEvent->watcher->data = obj;

	ssEvent->cb = ss_cb;
	ev_io_init(ssEvent->watcher, ssEvent->cb, ssEvent->fd, EV_READ);

	//web event with udp socket
	ssman_ioEvent* webEvent = &(event->ioObj[1]);
	webEvent->fd = createUdpSocket(config->web_port);
	if(webEvent->fd < 0)
	{
		_LOG("Create udp socket failed.");
		ssman_deinit(obj);
		return SS_ERR;
	}

	webEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(webEvent->watcher == NULL)
	{
		_LOG("Malloc ev_io failed.");
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
		_LOG("Malloc ev_timer failed.");
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
	if(obj == NULL || msg == NULL)
		return SS_ERR;
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
	if(json_obj==NULL)
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

int ssman_parseMsg_web(char* msg, ssman_obj* obj, char* result)
{
	if(result == NULL || obj == NULL || msg == NULL)
		return SS_ERR;
	
	strncpy(result,SS_ACK_ERR,sizeof(SS_ACK_ERR));

	//msg from web is a json file as show below
	//{
	//	"cmd"="xxx";
	//	"server_port"=1234;
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
	if(json_obj == NULL)
		return SS_ERR;

	if(json_obj->type != json_object)
	{
		json_value_free(json_obj);
		return SS_ERR;
	}

	//init value for json
	ssman_cmd_detail detail;
	char cmd[SS_CFG_OPT_SIZE_SMALL];
	memset(&detail,0,sizeof(ssman_cmd_detail));
	memset(cmd,0,SS_CFG_OPT_SIZE_SMALL);
	detail.config = obj->config;

	unsigned int i;
	for(i=0;i<json_obj->u.object.length;i++)
	{
		char* name = json_obj->u.object.values[i].name;
		json_value* value = json_obj->u.object.values[i].value;
		
		if(strcmp(name,"cmd")==0)
		{
			strncpy(cmd,value->u.string.ptr,SS_CFG_OPT_SIZE_SMALL);
			cmd[SS_CFG_OPT_SIZE_SMALL-1]='\0';
		}
		else if(strcmp(name,"server_port")==0)
			detail.server_port = value->u.integer;
		else if(strcmp(name,"password")==0)
		{
			strncpy(detail.password,value->u.string.ptr,SS_CFG_OPT_SIZE_SMALL);
			detail.password[SS_CFG_OPT_SIZE_SMALL-1]='\0';
		}
	}

	json_value_free(json_obj);

	char* systemCmd = NULL;

	//operate cmd
	if(strcmp(cmd,"add")==0)
	{
		_LOG("cmd: add");
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

		strncpy(result,SS_ACK_OK,sizeof(SS_ACK_OK));
	}
	else if(strcmp(cmd,"remove")==0)
	{
		_LOG("cmd: remove");
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
		char path[SS_PIDFILE_SIZE];
		snprintf(path,SS_PIDFILE_SIZE,"/tmp/.shadowsocks_%d.pid",detail.server_port);
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

		strncpy(result,SS_ACK_OK,sizeof(SS_ACK_OK));
	}
	else if(strcmp(cmd,"get") == 0)
	{
		_LOG("cmd: get");
		//check neccesary option indetail
		if(detail.server_port == 0)
			return SS_ERR;

		sshash_table* s = findPort(detail.server_port, &obj->portTable);
		if(s == NULL)
			return SS_ERR;
		
		snprintf(result,SS_RESULT_SIZE,"the port: %d dataUsage is: %f.",detail.server_port,(s->ctx.dataUsage/1024.0)/1024.0);
	}
	else if(strcmp(cmd,"status") == 0)
	{
		_LOG("cmd: status");
		//no need any ohter detail
		int num = countPort(&obj->portTable);
		snprintf(result,SS_RESULT_SIZE,"the port num is %d.",num);
	}
	else if(strcmp(cmd,"stop") == 0)
	{
		_LOG("cmd: stop");
		//stop event loop
		ev_break(obj->event->loop, EVBREAK_ALL);
		strncpy(result,SS_ACK_OK,sizeof(SS_ACK_OK));
	}
	else
		return SS_ERR;

	return SS_OK;
}


