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

int ssman_db_init(ssman_db_obj* obj)
{
	if(obj == NULL)
		return SS_ERR;

	//init config
	ssman_db_config* config = obj->conifg;

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

	sqlite3_close(obj->db);

	return SS_OK;
}

