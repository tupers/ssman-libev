#include "ssman_db.h"

//ev_io cb
static void ssman_cb(EV_P_ ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		_LOG("ssman cb error.");
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
	
	_LOG(buffer);

}

static void web_cb(EV_P_ ev_io* watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		_LOG("web cb error.");
		return;
	}

	//recv data
	
}

//sql cb
static int sql_checkTable_cb(void* arg, int num, char** ctx, char** colname)
{
	char* check = arg;
	int i;
	if(strcmp(ctx[0],"ipList")==0)
		check[0]=1;
	else if(strcmp(ctx[0],"portList")==0)
		check[1]=1;
	return 0;
}

static int isTableExisted(sqlite3* db)
{
	char check[2];
	memset(check,0,2);
	sqlite3_exec(db,SQL_LIST_TABLE,sql_checkTable_cb,check,NULL);
	int i;
	for(i=0;i<2;i++)
		if(check[i] == 0)
			return SS_ERR;
	return SS_OK;
}

static int sql_count_cb(void* arg, int num, char** ctx, char** colname)
{
	int* count = arg;
	*count = atoi(ctx[0]);

	return 0;
}

static int isIpExisted(sqlite3* db,char* ip)
{
	int check = 0;
	char cmd[SS_CFG_OPT_SIZE];
	snprintf(cmd,SS_CFG_OPT_SIZE,"select count(*) from ipList where ip = \'%s\';",ip);
	sqlite3_exec(db,cmd,sql_count_cb,&check,NULL);
	if(check)
		return SS_OK;
	else
		return SS_ERR;
}

static int sql_incrementCount_cb(void* arg, int num, char** ctx, char** colname)
{
	_LOG("here");
	int* delete_num = arg;
	*delete_num += 1;

	return 0;
}

static int isPortExisted(sqlite3* db,int port,int group)
{
	int check = 0;
	char cmd[SS_CFG_OPT_SIZE];
	snprintf(cmd,SS_CFG_OPT_SIZE,"select count(*) from portList where port = %d and ip_group = %d;",port,group);
	sqlite3_exec(db,cmd,sql_count_cb,&check,NULL);
	if(check)
		return SS_OK;
	else
		return SS_ERR;
}

static int sql_loadPortTable_cb(void* arg, int num, char** ctx, char** colname)
{
	_server_list* list = arg;
	_server_info* info = &(list->list[list->i]);
	int i;
	for(i=0;i<num;i++)
	{
		if(strcmp(colname[i],"port") == 0)
			info->port = atoi(ctx[i]);
		else if(strcmp(colname[i],"password") == 0)
		{
			strncpy(info->passwd,ctx[i],SS_CFG_OPT_SIZE_SMALL);
			info->passwd[SS_CFG_OPT_SIZE_SMALL-1] = '\0';
		}
		else if(strcmp(colname[i],"ip_group") == 0)
			info->group = atoi(ctx[i]);
		else if(strcmp(colname[i],"used") == 0)
			info->used = atoi(ctx[i]);
	}
	
	list->i += 1;
	
	return 0;
}

static char* createCmdJson(char* optstring, int server_port, char* passwd)
{
	static char cmd[SS_CMD_SIZE];
	memset(cmd,0,SS_CMD_SIZE);

	if(strcmp(optstring,"add") == 0)
	{
		if(server_port == 0 || passwd[0] == '\0')
			return NULL;
		snprintf(cmd,SS_CMD_SIZE,"{\"cmd\":\"add\",\"server_port\":%d,\"password\":\"%s\"}",server_port,passwd);
	}

	return cmd;
}

static int sql_sendTossman_cb(void* arg, int num, char** ctx, char** colname)
{
	_server_sendtossman_cb_str* info = arg;

	//if port used send cmd to ssman to open it in remote server
	if(info->info->used)
	{
		*(info->plan_num) += 1;

		info->addr->sin_addr.s_addr = inet_addr(ctx[0]);

		//send cmd
		char* cmd = createCmdJson("add",info->info->port,info->info->passwd);
		sendto(info->fd,cmd,strlen(cmd),0,(struct sockaddr*)info->addr, sizeof(struct sockaddr_in));

		//recv ack
		char ack[SS_CFG_OPT_SIZE_SMALL];
		strncpy(ack,SS_ACK_ERR,SS_CFG_OPT_SIZE_SMALL);
		struct sockaddr_in remoteAddr;
		remoteAddr.sin_family = AF_INET;
		socklen_t len = sizeof(remoteAddr);
		int ret = recvfrom(info->fd,ack,SS_CFG_OPT_SIZE_SMALL,0,(struct sockaddr*)&remoteAddr,&len);
		if(ret>0)
			ack[ret] = '\0';
		if(strcmp(ack,SS_ACK_OK) == 0)
			*(info->success_num) += 1;
	}

	return 0;
}

static int sql_debug_cb(void* arg, int num, char** ctx, char** colname)
{
	int i;
	for(i=0;i<num;i++)
		printf("[%s]:%s,",colname[i],ctx[i]);
	printf("\n");

	return 0;
}

int ssman_db_init(ssman_db_obj* obj)
{
	if(obj == NULL)
		return SS_ERR;

	//init config
	ssman_db_config* config = obj->config;
	config->sendcmd_fd = createUdpSocket(config->sendcmd_localPort);
	if(config->sendcmd_fd < 0)
	{
		_LOG("Create send cmd udp socket failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}
	config->ssmanAddr.sin_family = AF_INET;
	config->ssmanAddr.sin_port = htons(config->sendcmd_serverPort);
	struct timeval tv = {0,500000};
	setsockopt(config->sendcmd_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));

	//--------------------create event------------------------------------
	obj->event = (ssman_db_event*)malloc(sizeof(ssman_db_event));
	if(obj->event == NULL)
	{
		_LOG("Malloc ssman_db_init failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}

	//fill event struct
	ssman_db_event* event = obj->event;
	memset(event,0,sizeof(ssman_db_event));
	event->loop = EV_DEFAULT;
	event->ioObjNum = SS_DB_IOEVENT_NUM;
	event->ioObj = (ssman_ioEvent*)malloc(sizeof(ssman_ioEvent)*event->ioObjNum);
	if(event->ioObj == NULL)
	{
		_LOG("Malloc to event failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}
	memset(event->ioObj,0,sizeof(ssman_ioEvent)*event->ioObjNum);

	// event from ssman with udp ping
	ssman_ioEvent* ssmanEvent = &(event->ioObj[0]);
	ssmanEvent->fd = createUdpSocket(SS_DB_SSMAN_UDP_PORT);
	if(ssmanEvent->fd < 0)
	{
		_LOG("Create udp socket failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}

	ssmanEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(ssmanEvent->watcher == NULL)
	{
		_LOG("Malloc ev_io failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}
	ssmanEvent->watcher->data = obj;

	ssmanEvent->cb = ssman_cb;
	ev_io_init(ssmanEvent->watcher, ssmanEvent->cb, ssmanEvent->fd, EV_READ);

	//event from website with udp command
	ssman_ioEvent* webEvent = &(event->ioObj[1]);
	webEvent->fd = createUdpSocket(SS_DB_WEB_UDP_PORT);
	if(webEvent->fd < 0)
	{
		_LOG("Create udp socket failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}

	webEvent->watcher = (ev_io*)malloc(sizeof(ev_io));
	if(webEvent->watcher == NULL)
	{
		_LOG("Malloc ev_io failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}
	webEvent->watcher->data = obj;

	webEvent->cb = web_cb;
	ev_io_init(webEvent->watcher, webEvent->cb, webEvent->fd, EV_READ);
	
	//bind event with loop
	ev_io_start(event->loop, ssmanEvent->watcher);
	ev_io_start(event->loop, webEvent->watcher);

	//init db
	if(sqlite3_open(config->dbPath,&(obj->db)) != SQLITE_OK)
	{
		_LOG("Open database failed.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}
	
	//check table in db is properly created
	if(isTableExisted(obj->db) == SS_ERR)
	{
		_LOG("Tables not find in db.");
		ssman_db_deinit(obj);
		return SS_ERR;
	}

	return SS_OK;
}

void ssman_db_deinit(ssman_db_obj* obj)
{
	if(obj)
	{
		if(obj->config)
			free(obj->config);
		
		if(obj->event)
			free(obj->event);

		if(obj->db)
			sqlite3_close(obj->db);
	}
	memset(&obj,0,sizeof(ssman_db_obj));
}

void ssman_db_exec(ssman_db_obj* obj)
{
	ev_run(obj->event->loop,0);
}

int ssman_db_updateDb(char* ipList, char* configPath, char* dbPath)
{
	//open file and parse them as json file
	json_value* ip_json = openJsonConfig(ipList);
	json_value* cfg_json = openJsonConfig(configPath);

	if(ip_json == NULL || cfg_json == NULL)
	{
		_LOG("Can not open ip list or config file.");
		if(ip_json)closeJsonConfig(ip_json);
		if(cfg_json)closeJsonConfig(cfg_json);
		return SS_ERR;
	}
	
	//open db or create it
	sqlite3* db;
	if(sqlite3_open(dbPath,&db) != SQLITE_OK)
	{
		_LOG("Can not open database");
		closeJsonConfig(ip_json);
		closeJsonConfig(cfg_json);
		return SS_ERR;
	}
	
	sqlite3_exec(db,SQL_CREATE_IPLIST,NULL,NULL,NULL);
	sqlite3_exec(db,SQL_CREATE_PORTLIST,NULL,NULL,NULL);
	
	//check table in db is properly created
	if(isTableExisted(db) == SS_ERR)
	{
		_LOG("Ip list or port list are not in the database");
		closeJsonConfig(ip_json);
		closeJsonConfig(cfg_json);
		sqlite3_close(db);
		return SS_ERR;
	}

	//1. first check ip list
	int i;
	int update_num = 0;
	int delete_num = 0;
	int insert_num = 0;
	char cmd[SS_CFG_OPT_SIZE];
	sqlite3_exec(db,"update ipList set flag = 0;",NULL,NULL,NULL);

	for(i=0;i<ip_json->u.object.length;i++)
	{
		char* name = ip_json->u.object.values[i].name;
		json_value* item = ip_json->u.object.values[i].value;
		if(strcmp(name,"item")==0 && item->type == json_object)
		{
			//get ip and group from json file
			char ip[SS_CFG_OPT_SIZE];
			int group=-1;
			memset(ip,0,SS_CFG_OPT_SIZE);

			int j;
			for(j=0;j<item->u.object.length;j++)
			{
				char* item_name = item->u.object.values[j].name;
				json_value* item_value = item->u.object.values[j].value;
				
				if(strcmp(item_name,"ip") == 0)
				{
					strncpy(ip,item_value->u.string.ptr,SS_CFG_OPT_SIZE);
					ip[SS_CFG_OPT_SIZE-1] = '\0';
				}
				else if(strcmp(item_name,"group") == 0)
					group = item_value->u.integer;
			}
			//compare them with ip list
			if(group != -1 && ip[0]!='\0')
			{

				//first check
				if(isIpExisted(db,ip) == SS_ERR)
				{
					//not existed
					insert_num++;
					snprintf(cmd,SS_CFG_OPT_SIZE,"insert into ipList (ip,ip_group,flag) values (\'%s\',%d,%d);",ip,group,1);
					sqlite3_exec(db,cmd,NULL,NULL,NULL);
				}
				else
				{
					//if existed
					update_num++;
					snprintf(cmd,SS_CFG_OPT_SIZE,"update ipList set flag = 1 where ip = \'%s\';",ip);
					sqlite3_exec(db,cmd,NULL,NULL,NULL);
				}

			}
		}
	}
	
		
	//delete item not used
	sqlite3_exec(db,"select count(*) from ipList where flag = 0;",sql_count_cb,&delete_num,NULL);
	sqlite3_exec(db,"delete from ipList where flag = 0;",NULL,NULL,NULL);

	//iplist update report
	snprintf(cmd,SS_CFG_OPT_SIZE,"update ipList finished, update:%d, insert:%d, delete:%d.",update_num,insert_num,delete_num);
	_LOG(cmd);

	closeJsonConfig(ip_json);

	//2.then check port list
	sqlite3_exec(db,"update portList set flag = 0;",NULL,NULL,NULL);
	for(i=0;i<cfg_json->u.object.length;i++)
	{
		char* name = cfg_json->u.object.values[i].name;
		json_value* group = cfg_json->u.object.values[i].value;
		if(strcmp(name,"group") == 0 && group->type == json_object)
		{
			//get id start size from json file
			int id = -1;
			int start = -1;
			int size = 0;
			int j;
			for(j=0;j<group->u.object.length;j++)
			{
				char* group_name = group->u.object.values[j].name;
				json_value* group_value = group->u.object.values[j].value;

				if(strcmp(group_name,"id") == 0)
					id = group_value->u.integer;
				if(strcmp(group_name,"start") == 0)
					start = group_value->u.integer;
				if(strcmp(group_name,"size") == 0)
					size = group_value->u.integer;
			}
			//compare them with port list
			if(id != -1 && start != -1 && size != 0)
			{
				update_num = 0;
				insert_num = 0;
				delete_num = 0;
				
				//delete port out of range
				snprintf(cmd,SS_CFG_OPT_SIZE,"select count(*) from ipList where ip_group = %d and (port < %d or port >= %d);",id,start,start+size);
				sqlite3_exec(db,cmd,sql_count_cb,&delete_num,NULL);
				snprintf(cmd,SS_CFG_OPT_SIZE,"delete from ipList where ip_group = %d and (port < %d or port >= %d);",id,start,start+size);
				sqlite3_exec(db,cmd,NULL,NULL,NULL);

				//check port in range
				int k;
				for(k=start;k<start+size;k++)
				{
					if(isPortExisted(db,k,id) == SS_ERR)
					{
						//not existed
						insert_num++;
						snprintf(cmd,SS_CFG_OPT_SIZE,"insert into portList (port,ip_group,flag) values (%d,%d,%d);",k,id,1);
						sqlite3_exec(db,cmd,NULL,NULL,NULL);
					}
					else
					{
						update_num++;
						snprintf(cmd,SS_CFG_OPT_SIZE,"update portList set flag = 1 where port = %d and ip_group = %d;",k,id);
						sqlite3_exec(db,cmd,NULL,NULL,NULL);
					}
				}

				//portList update report
				snprintf(cmd,SS_CFG_OPT_SIZE,"update portList in group:%d finished, update:%d, insert:%d, delete:%d.",id,update_num,insert_num,delete_num);
				_LOG(cmd);
			}
		}
	}

	//delete port in group which not existed
	delete_num = 0;
	snprintf(cmd,SS_CFG_OPT_SIZE,"delete from portList where flag = 0;");
	sqlite3_exec(db,cmd,sql_incrementCount_cb,&delete_num,NULL);

	//portList update report
	snprintf(cmd,SS_CFG_OPT_SIZE,"update portList finished, delete:%d in group which not existed.",delete_num);
	_LOG(cmd);

	closeJsonConfig(cfg_json);

	//finish	
	sqlite3_close(db);

	return SS_OK;
}

int ssman_db_start(ssman_db_obj* obj)
{
	int i;
	char cmd[SS_CFG_OPT_SIZE];
	int num = 0;
	int plan_num = 0;
	int success_num = 0;
	sqlite3_exec(obj->db,"select count(*) from portList;",sql_count_cb,&num,NULL);

	_server_list port_list;
	port_list.i = 0;
	port_list.list = (_server_info*)malloc(sizeof(_server_info)*num);
	if(port_list.list== NULL)
	{
		_LOG("Malloc port list failed.");
		return SS_ERR;
	}
	
	sqlite3_exec(obj->db,"select port,password,ip_group,used from portList;",sql_loadPortTable_cb,&port_list,NULL);

	//ssman ipadrr struct
	//..
	
	_server_sendtossman_cb_str info;
	info.fd = obj->config->sendcmd_fd;
	info.plan_num = &plan_num;
	info.success_num = &success_num;
	info.addr = &(obj->config->ssmanAddr);

	for(i=0;i<num;i++)
	{
		info.info = &(port_list.list[i]);

		snprintf(cmd,SS_CFG_OPT_SIZE,"select ip from ipList where ip_group = %d;",info.info->group);
		sqlite3_exec(obj->db,cmd,sql_sendTossman_cb,&info,NULL);
	}

	free(port_list.list);

	//start report
	snprintf(cmd,SS_CFG_OPT_SIZE,"start remote ss-server finished, plan:%d, sucess:%d.",plan_num,success_num);
	_LOG(cmd);

	return SS_OK;
}
