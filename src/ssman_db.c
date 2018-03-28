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


int ssman_db_init(ssman_db_obj* obj)
{
	if(obj == NULL)
		return SS_ERR;

	//init config
	ssman_db_config* config = obj->config;

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

	sqlite3_exec(obj->db,SQL_CREATE_IPLIST,NULL,NULL,NULL);
	sqlite3_exec(obj->db,SQL_CREATE_PORTLIST,NULL,NULL,NULL);
	
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

int ssman_db_importIP(char* ipList, char* dbPath)
{
	//open json ip list
	json_value* json_obj = openJsonConfig(ipList);
	if(json_obj == NULL)
		return SS_ERR;

	sqlite3* db;

	//open db
	if(sqlite3_open(dbPath,&db) != SQLITE_OK)
	{
		_LOG("Import IP: open database failed.");
		closeJsonConfig(json_obj);
		return SS_ERR;
	}

	//check db
	if(isTableExisted(db) == SS_ERR)
	{
		_LOG("Import IP: wrong database.");
		sqlite3_close(db);
		closeJsonConfig(json_obj);
		return SS_ERR;
	}
	
	//clean ip table
	sqlite3_exec(db,"delete from ipList",NULL,NULL,NULL);	

	//insert
	int i;
	for(i=0;i<json_obj->u.object.length;i++)
	{
		char* name = json_obj->u.object.values[i].name;
		json_value* value = json_obj->u.object.values[i].value;
		if(strcmp(name,"item")==0 && value->type == json_object)
		{
			char ip[SS_CFG_OPT_SIZE];
			int group=-1;
			memset(ip,0,SS_CFG_OPT_SIZE);

			int j;
			for(j=0;j<value->u.object.length;j++)
			{
				char* item_name = value->u.object.values[j].name;
				json_value* item_value = value->u.object.values[j].value;
				
				if(strcmp(item_name,"ip") == 0)
				{
					strncpy(ip,item_value->u.string.ptr,SS_CFG_OPT_SIZE);
					ip[SS_CFG_OPT_SIZE-1] = '\0';
				}
				else if(strcmp(item_name,"group") == 0)
					group = item_value->u.integer;
			}

			if(group != -1 && ip[0]!='\0')
			{
				char cmd[SS_CFG_OPT_SIZE];
				snprintf(cmd,SS_CFG_OPT_SIZE,"insert into ipList values ( \"%s\", %d );",ip,group);
				sqlite3_exec(db,cmd,NULL,NULL,NULL);
			}
		}

	}

	closeJsonConfig(json_obj);
	sqlite3_close(db);

	return SS_OK;
}
