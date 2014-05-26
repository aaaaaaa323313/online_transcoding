/*Copyright (C) Guanyu@ntu
 *Author:Guanyu
 *Date:2014-5-25     
 */

#include "log.h"

char level[][10] = 
{
	"info",
	"debug",
	"warn",
	"error"
};

FILE* fplog[4] = {NULL};

static void getcurtime(char* buf, int len)
{
	struct tm* newtime;
	char* databuf = buf;
	time_t nowtime;
	time(&nowtime);
	newtime=localtime(&nowtime);
	strftime(databuf, len, "%g%m%d%H%M%S", newtime);
}

int creatlogfile()
{
	int i;
	char filename[FILE_NAME_LEN];
	char databuf[DATA_LEN];

	getcurtime(databuf, DATA_LEN);

	for (i = INFO; i <= ERROR; ++i)
	{
		strncpy(filename, SERV_NAME, 20);
		strncat(filename, "_", 1);
		strncat(filename, databuf, DATA_LEN);
		strncat(filename, "_", 1);
		strncat(filename, level[i], 20);
		strncat(filename, ".log", 20);
		fplog[i] = fopen(filename, "w+");
		printf(filename);
	}
	return 0;
}

int closelogfile()
{
	int i;
	for (i = INFO; i <= ERROR; ++i)
		fclose(fplog[i]);
	return 0;
}


int servlog(TYPE type, const char* format, ...)
{
	assert(type >= INFO && type <= ERROR);
	char	logbuf[LOG_MAX_LEN];
	char	line[MAX_LINE_LEN];
	char	databuf[DATA_LEN];
	va_list argptr; 
	int		cnt;

	memset(line, 0, MAX_LINE_LEN);
	va_start(argptr, format);
	cnt = vsnprintf(logbuf, LOG_MAX_LEN, format, argptr);
	va_end(argptr);

	getcurtime(databuf, DATA_LEN);
	strncpy(line, databuf, DATA_LEN);
	strncat(line, ":"    , 1);
	strncat(line, logbuf , cnt);
	strncat(line, "\n"   , 1);

	fwrite(line, 1, strlen(line), fplog[type]);

//#ifdef _DEBUG
	fflush(fplog[type]);
//#endif

	return 0;
}

/*Usage
int main(void)
{
	creatlogfile();

	servlog(ERROR, "%s %d", "hello", 1);
	servlog(DEBUG, "%s %s", "debug", "guanyu");
	servlog(DEBUG, "%s", "hello world");

	closelogfile();
	return 0; 
} */
