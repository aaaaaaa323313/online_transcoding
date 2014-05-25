/*Copyright (C) Wison Telecommunication Tech, Ltd
 *Author:Guanyu
 *Date:2011-3-5     
 */

#ifndef SERV_LOG_HH
#define SERV_LOG_HH

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#define LOG_MAX_LEN		400
#define MAX_LINE_LEN	450
#define DATA_LEN		20
#define FILE_NAME_LEN	100
#define SERV_NAME		"fsys"

typedef enum _TYPE
{
	INFO	= 0,	//it must start with zero
	DEBUG,
	WARN,
	ERROR
}TYPE;


int creatlogfile();
int servlog(TYPE type, const char* format, ...);
int closelogfile();

#endif



