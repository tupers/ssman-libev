#include "ssman.h"

ssman::ssman()
{
	
}

ssman::~ssman()
{
	if(m_fdWebSock!=-1)
		close(m_fdWebSock);

	if(m_fdSSSock!=-1)
		close(m_fdSSSock);

	for(auto io : m_vecWatcher)
		delete io;
}

int ssman::createUnixSocket(string path)
{
	const char* dir=path.c_str();
	unlink(dir);
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(fd<0)
		return -1;
	
	struct sockaddr_un addr;
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path.c_str());
	int len = offsetof(struct sockaddr_un, sun_path) + path.size();

	if(bind(fd,(struct sockaddr*)&addr, len)<0)
	{
		close(fd);
		return -1;
	}

	return fd;
}

int ssman::createUdpSocket(int port)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd<0)
		return -1;
	
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(fd,(struct sockaddr*)&addr, sizeof(addr))<0)
	{
		close(fd);
		return -1;
	}

	return fd;
}

void ssman::webCb(ev::io &w, int revents)
{
	char buffer[BUFFER_SIZE];
	struct sockaddr_un remoteAddr;

	if(EV_ERROR & revents)
		cout<<"UNIX got invalid event "<<endl;

	socklen_t remoteAddr_len = sizeof(struct sockaddr_un);
	int len = recvfrom(w.fd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&remoteAddr,(socklen_t*)&remoteAddr_len);
	buffer[len]='\0';
	cout<<buffer<<endl;
}

void ssman::ssCb(ev::io &w, int revents)
{
	char buffer[BUFFER_SIZE];
	struct sockaddr_in remoteAddr;

	if(EV_ERROR & revents)
		cout<<"UDP got invalid event"<<endl;

	socklen_t remoteAddr_len = sizeof(remoteAddr);

	int len = recvfrom(w.fd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&remoteAddr,&remoteAddr_len);
	buffer[len]='\0';

	char msg[]="ok\n";
	sendto(w.fd,msg,sizeof(msg),0,(struct sockaddr*)&remoteAddr,sizeof(remoteAddr));
}

int ssman::init()
{
	//create sockets
	m_fdSSSock = createUnixSocket(UNIX_SOCK);
	if(m_fdSSSock<0)
		return -1;
	m_fdWebSock = createUdpSocket(UDP_PORT);
	if(m_fdWebSock<0)
	{
		close(m_fdSSSock);
		return -2;
	}

	//one for unix and one for udp
	for(int i=0;i<2;i++)
	{
		ev::io* watcher = new ev::io;
		m_vecWatcher.push_back(watcher);
	}
	
	//set up watcher into loop
		
	//first unix sock watcher
	m_vecWatcher[0]->set(m_loop);
	m_vecWatcher[0]->set(m_fdSSSock,ev::READ);
	m_vecWatcher[0]->set<ssman,&ssman::webCb>(this);
	m_vecWatcher[0]->start();

	//then udp sock watcher
	m_vecWatcher[1]->set(m_loop);
	m_vecWatcher[1]->set(m_fdWebSock,ev::READ);
	m_vecWatcher[1]->set<ssman,&ssman::ssCb>(this);
	m_vecWatcher[1]->start();
	return 0;
}

void ssman::exec()
{
	m_loop.run(0);
}

int main()
{
	ssman man;
	auto ret = man.init();
	if(ret<0)
	{
		cout<<"init err: "<<ret<<endl;
		return -1;
	}
	man.exec();	
}
