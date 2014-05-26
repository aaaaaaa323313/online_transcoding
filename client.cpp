#include "client.h"
#include "fsys.h"
#include "log.h"


int get_unique_id(const char *path, char *file_id);

bool trans_file(const TransTask& task, bool sync, int priority)
{
  shared_ptr<TTransport> socket(new TSocket("localhost", 9190));
  shared_ptr<TTransport> transport(new TFramedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  TranscoderClient client(protocol);
  bool ret = false;

  try 
  {
    transport->open();
	ret = client.trans(task, sync, priority);
	transport->close();
  } 
  catch (TException &tx) 
  {
	servlog(ERROR, "trans_file() ERROR: %s\n", tx.what());
	ret = false;
  }
  return ret;

}


int query_db(TransTask& task)
{
  shared_ptr<TTransport> socket(new TSocket("localhost", 9191));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  FileInfoDBClient client(protocol);
  FileInfo fileinfo;
  int ret = -1;

  try 
  {
    transport->open();
	servlog(INFO, "query_db() task.filename    = %s", task.filename.c_str());
	servlog(INFO, "query_db() task.interval    = %d", task.interval);
	servlog(INFO, "query_db() task.vcodec_type = %s", task.vcodec_type.c_str());
	servlog(INFO, "query_db() task.dest_type,  = %s", task.dest_type.c_str());

	char file_id[30];
	int fileid = get_unique_id(task.filename.c_str(), file_id);
	if(fileid < 0)
		return -1;

	servlog(INFO, "query_db() file unique id = %s", file_id);

	client.getFileInfo(fileinfo, file_id);
    transport->close();

	servlog(INFO, "query_db() fileinfo.success     = %d", fileinfo.success);
	servlog(INFO, "query_db() fileinfo.play_time   = %f", fileinfo.play_time);
	servlog(INFO, "query_db() fileinfo.source      = %s", fileinfo.source.c_str());
	servlog(INFO, "query_db() fileinfo.source_type = %s", fileinfo.source_type.c_str());
	if (fileinfo.success == false)
		return -1;
	task.dest_type = "ts";
	task.interval  = 10;
	task.source = fileinfo.source;
	task.source_type = fileinfo.source_type;
	task.play_time = fileinfo.play_time;
	ret = 0;
  } 
  catch (TException &tx) 
  {
    servlog(ERROR, "query_db() ERROR: %s\n", tx.what());
	ret = -1;
  }

  return ret;

}




int if_file_exist(const char *path)
{
	int ret = access(path, F_OK);
	if (!ret)
		return 1;		//file exist
	else if ((ret == -1)&& (errno == ENOENT))
		return 0;		//file not exist

	return -1;			//error

}

int add_url(const std::string& url)
{  

	shared_ptr<TTransport> socket(new TSocket("localhost", 9191));
	shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	FileInfoDBClient client(protocol);
	int ret = -1;

	try 
	{
		transport->open();
		ret = client.add(url);
		transport->close();
	} 
	catch (TException &tx) 
	{
		servlog(ERROR, "add_url()  ERROR: %s\n", tx.what());
		ret = -1;
	}
	return ret;

}

int get_name_info(const char *path, TransTask& task)
{

	char Param[7][30];

	int len = strlen(path);
	while (--len >= 0)
		if (path[len] == '/')
			break;

	if (len < 0)
		return -1;

	int i;
	int j = 0;
	int index = 0;

	for (i = ++len; i < strlen(path); i++)  
	{                           
		if (path[i] != '_'&& path[i] != '.'&& path[i] != '-')             
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

			if (index == 7 && strstr(path, ".ts"))
				break;
			if (index == 6 && strstr(path, ".m3u8"))
				break;
		}
	}

	if (index == 6 && strstr(path, ".m3u8"))
	{
		task.server_ip		= "";					//在生成的m3u8文件中，不显示IP字段 
		task.filename		= string(path);
		task.vcodec_type	= string(Param[1]);
		task.width			= string(Param[2]);
		task.height			= string(Param[3]);
		task.vbitrate		= atoi(Param[4]);
		task.abitrate		= atoi(Param[5]);
		return 0;
	}
	
	if (index == 7 && strstr(path, ".ts"))
	{
		task.server_ip		= "";					//在生成的m3u8文件中，不显示IP字段
		task.filename		= string(path);
		task.vcodec_type	= string(Param[1]);
		task.width			= string(Param[2]);
		task.height			= string(Param[3]);
		task.vbitrate		= atoi(Param[4]);
		task.abitrate		= atoi(Param[5]);
		task.interval		= atoi(Param[6]);
		return 0;
	}
	return -1;

}


int judge_file_suffix(const char *path)
{
	if (!(strstr(path, ".m3u8") || strstr(path, ".ts")))
		return 0;
	else
		return 1;
}


int get_unique_id(const char *path, char *file_id)
{

	char Param[30];

	int len = strlen(path);
	while (--len >= 0)
		if (path[len] == '/')
			break;

	if (len < 0)
		return -1;

	int i;
	int j = 0;

	for (i = ++len; i < strlen(path); i++)  
	{                           
		if (path[i] != '_'&& path[i] != '.'&& path[i] != '-')             
		{                        
			Param[j] = path[i];
			j++;
		}
		else
		{
			Param[j] = '\0';
			break;
		}
	}
	strcpy(file_id, Param);
	return atoi(Param);

}

/*
 *This function is now not used in the program.
 *May be added in in the futhure
 */
int creat_target_file(const char *path)
{
	int ret;
	//if the file is .m3u8 or .ts
	ret = judge_file_suffix(path); //0 the file is neither m3u8 nor ts
	if (!ret)
		return 0; 
	
	ret = if_file_exist(path); // -1 error; 0 not exist; 1 exist;
	if (ret == -1)
		return -1;
	else if (ret == 0)
	{
		FILE *fp;
		fp = fopen(path, "w+");
		fclose(fp);
	}
	return 0;
}



