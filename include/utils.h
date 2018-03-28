#ifndef _UTILS_H
#define _UTILS_H value

#include "sstype.h"
#include "json.h"

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

extern FILE* __g_logfile;
#define _LOG_FILE __g_logfile

#define _LOG_CLOSE								\
	do{									\
		if(_LOG_FILE){fclose(_LOG_FILE);}}				\
	while(0)

#define _LOG(msg) 									\
	do{									\
		if(_LOG_FILE){							\
			time_t now = time(NULL);				\
			char timestr[20];					\
			strftime(timestr,20,TIME_FORMAT,localtime(&now));	\
			fprintf(_LOG_FILE,"[%s]\t%s\n",timestr,msg);		\
			fflush(_LOG_FILE);}					\
		else{printf("%s\n",msg);}}					\
	while(0)

int createUnixSocket(const char* path);
int createUdpSocket(int port);
void daemonize(char* path);
json_value* openJsonConfig(char* cfgPath);
void closeJsonConfig(json_value* obj);

#endif /* ifndef _UTILS_H */
