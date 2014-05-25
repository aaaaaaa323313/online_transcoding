/*Copyright (C) Guanyu@ntu
 *Author:Guanyu
 *Date:2014-5-25			
 */
#include <syslog.h>

/* Global program parameters */
struct fsys_params {
    const char *basepath;
    int debug;
};

int judge_file_suffix(const char *path); //0 the file is neither m3u8 nor ts
                                         //1 the file is m3u8 or ts


int if_file_exist(const char *path); // -1 error; 0 not exist; 1 exist;
