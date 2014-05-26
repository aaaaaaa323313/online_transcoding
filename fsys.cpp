/*Copyright (C) Guanyu@ntu
 *Author:Guanyu
 *Date:2014-5-25         
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stddef.h>

#include "fsys.h"
#include "log.h"



//C99标准
struct fsys_params params = 
{
	params.basepath		  = NULL,
	params.debug          = 0
};

enum {
    KEY_HELP,
    KEY_VERSION,
    KEY_KEEP_OPT
};

#define FSYS_OPT(t, p, v) { t, offsetof(struct fsys_params, p), v }

static struct fuse_opt fsys_opts[] = {
    FSYS_OPT("-d",               debug, 1),
    FSYS_OPT("debug",            debug, 1),

    FUSE_OPT_KEY("-h",            KEY_HELP),
    FUSE_OPT_KEY("--help",        KEY_HELP),
    FUSE_OPT_KEY("-V",            KEY_VERSION),
    FUSE_OPT_KEY("--version",     KEY_VERSION),
    FUSE_OPT_KEY("-d",            KEY_KEEP_OPT),
    FUSE_OPT_KEY("debug",         KEY_KEEP_OPT),
    FUSE_OPT_END
};

/*
 * Translate file names from FUSE to the original absolute path. A buffer
 * is allocated using malloc for the translated path. It is the caller's
 * responsibility to free it.
 */

char* translate_path(const char* path) 
{
    printf("the file path is %s\n", path);

    char* result;
    result = (char *)malloc(strlen(params.basepath) + strlen(path) + 1);

    if (result) 
	{
        strcpy(result, params.basepath);
		//filte the '\'
		if (params.basepath[strlen(params.basepath)-1] == '/' 
				&& path[0] == '/')
			strcat(result, path+1);
		else
			strcat(result, path);
    }

    return result;
}

/* to judge if the requested file is a ts file
 * if the returned value is zero, then it's not a ts file
 * if the returned value is one, then it's a ts file
 */


int judge_file_suffix(const char *path)
{
    if (!(strstr(path, ".ts")))
        return 0;
    else
        return 1;
}

/* if the file has been existed */

int if_file_exist(const char *path)
{
    int ret = access(path, F_OK);
    if (!ret)
    {
        printf("the file exists in the path: %s\n", path);
        return 1;       //file exist
    }
    else if ((ret == -1) && (errno == ENOENT))
        return 0;       //file not exist

    return -1;          //error
}

int get_name_info(const char *path, TransTask* task)
{

	char Param[4][100] = {0};

	int len = strlen(path);
	while (--len >= 0)
		if (path[len] == '/')
			break;

	if (len < 0)
		return -1;

	unsigned int i;
	unsigned int j = 0;
	unsigned int index = 0;

	for (i = ++len; i < strlen(path); i++)  
	{                           
		if (path[i] != '_' && path[i] != '.' && path[i] != '-')             
		{                        
			Param[index][j] = path[i];
			j++;
		}
		else
		{
			Param[index][j] = '\0';
			servlog(INFO, "file name param:%s", Param[index]);
			index++;
			j = 0;

			if (index == 4 && path[i] == '.')
				break;	
		}
	}

		
	if (index == 4)
	{
		task -> width		= atoi(Param[1]); // width
		task -> height		= atoi(Param[2]); // height
		task -> bitrate		= atoi(Param[3]); // bitrate
        strcpy(task->file_name, Param[0]);    // the file name of the original ts file
		return 0;
	}
	return -1;

}


int transcode(const char *path)
{
	int ret;

	//if the file is a ts file
	ret = judge_file_suffix(path); //0, the file is not a ts file
	if (!ret)
		return 0;

	ret = if_file_exist(path); // -1 error; 0 not exist; 1 exist;
	if (ret == -1)
		return -1;
	else if (ret == 0)
	{
		TransTask task;
		ret = get_name_info(path, &task); //get the information from the file name.
		if (ret == -1)
		{
			servlog(ERROR, "can't get the info from file name:%s", path);
			return -1;
		}

		servlog(INFO, "task->file_name:%s", task.file_name);
		

		if(trans_file(task))
			servlog(INFO, "transcode the file success:%s", path);
		else
		{
			servlog(ERROR, "fail to transcode the file:%s", path);
			return -1;
		}
	}
	return 0;
}

int trans_file(TransTask task)
{
    return 1;
    return 0;
}

static int fsys_readlink(const char *path, char *buf, size_t size) 
{
    char* origpath;
    ssize_t len;


    errno = 0;

    origpath = translate_path(path);
    if (!origpath)
		return -errno;

    len = readlink(origpath, buf, size - 1);
    if (len == -1) 
	{
		free(origpath);
		return -errno;
    }

    buf[len] = '\0';
    free(origpath);
    return -errno;

}

static int fsys_access(const char *path, int mode)
{
	char* origpath;

    errno = 0;
    origpath = translate_path(path);
    if (!origpath)
		return -errno;
	return access(origpath, mode);

}

static int fsys_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
											off_t offset, struct fuse_file_info *fi) 
{
    char* origpath;
    DIR *dp;
    struct dirent *de;


    errno = 0;

    origpath = translate_path(path);
    if (!origpath)
		return -errno;

    dp = opendir(origpath);
    if (!dp) 
	{
		free(origpath);
		return -errno;
    }

    while ((de = readdir(dp))) 
	{
        struct stat st;
        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0)) 
			break;
    }

    closedir(dp);
    free(origpath);
    return -errno;
}

static int fsys_getattr(const char *path, struct stat *stbuf) 
{
    char* origpath;


    errno = 0;
    origpath = translate_path(path);
    if (!origpath)
		return -errno;
	
	transcode(origpath); //transcode the file

    /* pass-through for regular files */
    if (lstat(origpath, stbuf) == 0) 
	{
		free(origpath);
		return -errno;
    } 
	else 
        /* Not really an error. */
        errno = 0;

    if (lstat(origpath, stbuf) == -1) 
	{
		free(origpath);
		return -errno;
    }

    /*
     * Get size for resulting mp3 from regular file, otherwise it's a
     * symbolic link. */
    if (S_ISREG(stbuf->st_mode)) {
//        trans = transcoder_new(origpath);
//        if (!trans) {
//            goto transcoder_fail;
//        }

  //      stbuf->st_size = trans->totalsize;
  //      stbuf->st_blocks = (stbuf->st_size + 512 - 1) / 512;

//        transcoder_finish(trans);
//        transcoder_delete(trans);
    }

    free(origpath);
    return -errno;
}

static int fsys_open(const char *path, struct fuse_file_info *fi) 
{
    char* origpath;
    int fd;


    errno = 0;

    origpath = translate_path(path);
    if (!origpath) 
		return -errno;
	//transcoder the file
	transcode(origpath);


    fd = open(origpath, fi->flags);
    /* File does exist, but can't be opened. */
    if (fd == -1 && errno != ENOENT) 
	{
		free(origpath);
		return -errno;
    } 
	else 
        /* Not really an error. */
        errno = 0;

    /* File is real and can be opened. */
    if (fd != -1) 
	{
        close(fd);
		free(origpath);
		return -errno;
    }
	return -errno;
}

static int fsys_read(const char *path, char *buf, size_t size, off_t offset,
								struct fuse_file_info *fi) 
{
    char* origpath;
    int fd;
    int read = 0;

    servlog(INFO, "read %s: %zu bytes from %jd", path, size, offset);

    errno = 0;

    origpath = translate_path(path);
    
    servlog(INFO, "the original file is %s\n", origpath);

    if (!origpath) 
        return -errno;

	transcode(origpath);

    /* If this is a real file, pass the call through. */
    fd = open(origpath, O_RDONLY);
    if (fd != -1) 
	{
        read = pread(fd, buf, size, offset);
        close(fd);
		free(origpath);
		if (read)
			return read;
		else 
			return -errno;
    }

    /* File does exist, but can't be opened. */
    if (fd == -1 && errno != ENOENT) 
	{
		free(origpath);
        return -errno;
    }
	
	free(origpath);
    return -errno;
}

static int fsys_statfs(const char *path, struct statvfs *stbuf) 
{
    char* origpath;


    errno = 0;

    origpath = translate_path(path);
    if (!origpath) 
		return -errno;

    statvfs(origpath, stbuf);

    free(origpath);
    return -errno;
}

static int fsys_chmod(const char *path, mode_t mode)
{
	int res;
    char* origpath;

    origpath = translate_path(path);
    if (!origpath)
		return -errno;

	res = chmod(origpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int fsys_release(const char *path, struct fuse_file_info *fi) 
{
    return 0;
}

/* We need synchronous reads. */
static void *fsys_init(struct fuse_conn_info *conn) 
{
	//conn->async_read = 0;
    return NULL;
}


static struct fuse_operations fsys_ops = 
{
	fsys_ops.getattr  = fsys_getattr,
	fsys_ops.readlink = fsys_readlink,
	fsys_ops.getdir,
	fsys_ops.mknod,
	fsys_ops.mkdir,
	fsys_ops.unlink,
	fsys_ops.rmdir,
	fsys_ops.symlink,
	fsys_ops.rename,
	fsys_ops.link,
	fsys_ops.chmod	  = fsys_chmod,
	fsys_ops.chown,
	fsys_ops.truncate,
	fsys_ops.utime,
	fsys_ops.open     = fsys_open,
	fsys_ops.read     = fsys_read,
	fsys_ops.write,
	fsys_ops.statfs   = fsys_statfs,
	fsys_ops.flush,
	fsys_ops.release  = fsys_release,
	fsys_ops.fsync,
	fsys_ops.setxattr,
	fsys_ops.getxattr,
	fsys_ops.listxattr,
	fsys_ops.removexattr,
	fsys_ops.opendir,
	fsys_ops.readdir  = fsys_readdir,
	fsys_ops.releasedir,
	fsys_ops.fsyncdir,
	fsys_ops.init     = fsys_init,
	fsys_ops.destroy,
	fsys_ops.access	  = fsys_access,
	fsys_ops.create,
	fsys_ops.ftruncate,
	fsys_ops.fgetattr,
	fsys_ops.lock,
	fsys_ops.utimens,
	fsys_ops.bmap,
	fsys_ops.flag_nullpath_ok,
	fsys_ops.flag_reserved,
	fsys_ops.ioctl,
	fsys_ops.poll
};

void usage(char *name) 
{
    printf("Usage: %s [OPTION]... REALDIR MIRRORDIR\n", name);
    fputs("\
Mount REALDIR on MIRRORDIR, converting media files to FLV upon access.\n\
\n\
General options:\n\
	-h, --help             display this help and exit\n\
	-V, --version          output version information and exit\n\
\n", stdout);
}

static int fsys_opt_proc(void *data, const char *arg, int key,
											struct fuse_args *outargs) 
{
    switch(key) {
        case FUSE_OPT_KEY_NONOPT:
            // check for flacdir and bitrate parameters
            if (!params.basepath) 
			{
                params.basepath = arg;
                return 0;
            }
            break;

        case KEY_HELP:
            usage(outargs->argv[0]);
            fuse_opt_add_arg(outargs, "-ho");
            fuse_main(outargs->argc, outargs->argv, &fsys_ops, NULL);
            exit(1);

        case KEY_VERSION:
            printf("FSYS version %s\n", PACKAGE_VERSION);
            fuse_opt_add_arg(outargs, "--version");
            fuse_main(outargs->argc, outargs->argv, &fsys_ops, NULL);
            exit(0);
    }

    return 1;
}

int main(int argc, char *argv[]) 
{
    int ret;

	//create the log
	creatlogfile();
	servlog(INFO, "%s", "fsys start");

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &params, fsys_opts, fsys_opt_proc)) 
	{
        fprintf(stderr, "Error parsing options.\n\n");
        usage(argv[0]);
        return 1;
    }

    if (!params.basepath) 
	{
        fprintf(stderr, "No valid flacdir specified.\n\n");
        usage(argv[0]);
        return 1;
    }

    if (params.basepath[0] != '/') 
	{
        fprintf(stderr, "flacdir must be an absolute path.\n\n");
        usage(argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(params.basepath, &st) != 0 || !S_ISDIR(st.st_mode)) 
	{
        fprintf(stderr, "flacdir is not a valid directory: %s\n",
                params.basepath);
        fprintf(stderr, "Hint: Did you specify bitrate using the old "
                "syntax instead of the new -b?\n\n");
        usage(argv[0]);
        return 1;
    }

    /* Log to the screen if debug is enabled. */
    openlog("fsys", params.debug ? LOG_PERROR : 0, LOG_USER);

    servlog(INFO, "FSYS options:basepath->%s", params.basepath);

    // start FUSE
	umask(0);
    ret = fuse_main(args.argc, args.argv, &fsys_ops, NULL);

    fuse_opt_free_args(&args);

	servlog(INFO, "%s", "fsys have quited out");
	closelog();

    return ret;
}
