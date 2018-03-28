#include "utils.h"

FILE* __g_logfile = NULL;

//socket create function;
int createUnixSocket(const char* path)
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

int createUdpSocket(int port)
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

void daemonize(char* path)
{
	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if(pid<0)
	{
		printf("Fork failed.\n");
		exit(EXIT_FAILURE);
	}

	/* If we got a good PID, then
	 * we can exit the parent process. */
	if(pid>0)
	{
		FILE* file = fopen(path, "w");
		if(file == NULL)
		{
			perror("Invalid pid file path.");
			exit(EXIT_FAILURE);
		}


		fprintf(file, "%d", (int)pid);
		fclose(file);
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);

	//log
	//FILE* fd = fopen("/tmp/sslog","w");
	
	/* Create a new SID for the child process */
	sid = setsid();
	if(sid<0)
	{
		_LOG("setsid error.");
		_LOG_CLOSE;
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
	if((chdir("/"))<0)
	{
		_LOG("change directory error.");
		_LOG_CLOSE;
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

json_value* openJsonConfig(char* cfgPath)
{
	FILE* fd = fopen(cfgPath,"r");
	if(fd == NULL)
		return NULL;
	//open config file
	char* ctx = (char*)malloc(sizeof(char)*SS_CFG_SIZE);
	if(ctx == NULL)
	{
		fclose(fd);
		return NULL;
	}

	//check file size
	fseek(fd,0,SEEK_END);
	int len = ftell(fd);
	if(len>SS_CFG_SIZE)
	{
		free(ctx);
		fclose(fd);
		return NULL;
	}

	fseek(fd,0,SEEK_SET);
	fread(ctx,1,SS_CFG_SIZE,fd);
	fclose(fd);

	//transmit file into json obj
	json_value* obj = json_parse(ctx,len);
	if(obj == NULL)
	{
		free(ctx);
		return NULL;
	}

	if(obj->type != json_object)
	{
		json_value_free(obj);
		free(ctx);
		return NULL;
	}

	free(ctx);

	return obj;
}

void closeJsonConfig(json_value* obj)
{
	if(obj)
		json_value_free(obj);
}
