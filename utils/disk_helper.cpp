#include "disk_helper.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include "log/logger.h"
#include "utils/string_helper.h"

using namespace lin_io;

FileAttribute::FileAttribute()
{
    Reset();
}

FileAttribute::~FileAttribute()
{

}

void FileAttribute::Reset()
{
    m_fileName = "";
    m_fileDir = "";
    m_fileSize = 0;
    m_lastModifyTime = time(NULL); 
}

DiskHelper::DiskHelper()
{

}

DiskHelper::~DiskHelper()
{

}

bool DiskHelper::FileExist(const string& file_full_name)
{
    if(file_full_name.empty())
        return false;

	if(access(file_full_name.c_str(), 0) != 0)//0 exist
	{
		//GLERROR << file_full_name << " access error: " << strerror(errno);
		return false;
	}
	
    return true;
}

bool DiskHelper::FileInformation(const string& file_full_name, FileAttribute& file_attr)
{
	if(!FileExist(file_full_name))
		return false;

	struct stat fileStat;
	if(stat(file_full_name.c_str(), &fileStat) == 0)
	{
		if(S_ISREG(fileStat.st_mode)) 
		{
			string::size_type len = file_full_name.find_last_not_of(SPLITER);
			file_attr.m_fileName = file_full_name.substr(file_full_name.size() - len, len);
			file_attr.m_fileDir = file_full_name.substr(0, file_full_name.size() - len - 1);
			file_attr.m_fileSize = fileStat.st_size;
			file_attr.m_lastModifyTime = fileStat.st_mtime;
			return true;
		}
		GLERROR << file_full_name << " S_ISREG error: " << strerror(errno);
	}

	GLERROR << file_full_name << " stat error: " << strerror(errno);

	return false;
}

bool DiskHelper::OpenFile(const string& file_full_name,string& file_data)
{
	if(file_full_name.empty())
		return false;

	FILE* file = fopen(file_full_name.c_str(), "r");
	if(file == NULL)
	{
		GLERROR << file_full_name << " fopen error: " << strerror(errno);
		return false;
	}

	fseek(file, 0, SEEK_END);
	int dataLen = ftell(file);
	if (dataLen <= 0)
	{
		GLERROR << file_full_name << " ftell error: " << strerror(errno);
		fclose(file);
		return false;
	}
	fseek(file, 0, SEEK_SET);

	char* buf = new char[dataLen + 2];
	memset(buf, 0, dataLen + 2);
	int rtn = fread(buf, dataLen, 1, file);
	if(rtn < 1)
	{
		GLERROR << file_full_name << " fread error: " << strerror(errno);
		fclose(file);
		delete[] buf;
		return false;
	}

	file_data.assign(buf,dataLen);
	delete[] buf;
	fclose(file);
	return true;
}

bool DiskHelper::CreateFile(const string& file_full_name,const char* file_mode,const char* file_data,const int data_len)
{
	if(file_full_name.empty())
		return false;

	FILE* file = fopen(file_full_name.c_str(), file_mode);
	if(file == NULL)
	{
		GLERROR << file_full_name << " fopen error: " << strerror(errno);
		return false;
	}
	
	if (file_data != NULL && data_len > 0)
	{
		fwrite(file_data, 1, data_len, file);
		fflush(file);
	}
	fclose(file);
	return true;
}


bool DiskHelper::CopyFile(const string& src_file_full_name, const string& dst_file_full_name)
{
	if(!FileExist(src_file_full_name))
		return false;

	if(rename(src_file_full_name.c_str(), dst_file_full_name.c_str()))
	{
		GLERROR << src_file_full_name << " to " <<  dst_file_full_name << " rename error: " << strerror(errno);

		return false;
	}
	return true;

}

bool DiskHelper::CopyFiles(vector<string>& src_file_full_names, const string& dst_dir_full_name)
{
	for(vector<string>::iterator iter = src_file_full_names.begin(); iter != src_file_full_names.end(); iter++)    
	{
		if(!CopyDirectory(*iter, dst_dir_full_name))
		{
			return false;
		}
	}
	return true;
}

bool DiskHelper::RemoveFile(const string& file_full_name)
{
	if(!FileExist(file_full_name))
		return false;

	if(remove(file_full_name.c_str()) != 0)
	{
		GLERROR << file_full_name << " remove error: " << strerror(errno);

		return false;
	}
	return true;
}

bool DiskHelper::RemoveFiles(vector<string>& file_full_names)
{
	for(vector<string>::iterator iter = file_full_names.begin(); iter != file_full_names.end(); iter++)    
	{
		if(!RemoveFile(*iter))
		{
			return false;
		}
	}
	return true;
}

bool DiskHelper::DirectoryExist(const string& dir_full_name)
{
    if(dir_full_name.empty())
        return false;

	DIR* dir = opendir(dir_full_name.c_str());
	if(NULL == dir)
	{
		//GLERROR << dir_full_name << " opendir error: " << strerror(errno);
		return false;
	}
	closedir(dir);

	return true;
}

bool DiskHelper::CreateDirectory(const string& dir_full_name,const bool is_dir_tree)
{
	if(DirectoryExist(dir_full_name))
		return true;

	if (!is_dir_tree)
	{
		if(mkdir(dir_full_name.c_str(), POWER) != 0)
		{
			if(errno != EEXIST) 
			{
				GLERROR << dir_full_name << " mkdir error: " << strerror(errno);
				return false;
			}
		}
		return true;
	}
	
	string dirName = "";
	string dirFullName(dir_full_name);
	strHelper::Replace(dirFullName,'\\','/');
	vector<string> dirTree = strHelper::Split(dirFullName,'/');

	for(vector<string>::iterator iter = dirTree.begin(); iter != dirTree.end(); iter++)
	{
		if(*iter == ".")
		{
			dirName += *iter;
			continue;
		}
		if(*iter == "..")
		{
			dirName += *iter;
			continue;
		}
		dirName += SPLITER + *iter;

		if(mkdir(dirName.c_str(), POWER) != 0)
		{
			if(errno != EEXIST) 
			{
				GLERROR << dirFullName << " mkdir error: " << strerror(errno);
				return false;
			}
		}
	}
	return true;
}

bool DiskHelper::CopyDirectory(const string& src_dir_full_name, const string& dst_dir_full_name)
{
	if(!DirectoryExist(src_dir_full_name))
		return false;

	if(rename(src_dir_full_name.c_str(), dst_dir_full_name.c_str()))
	{
		GLERROR << src_dir_full_name << " to " <<  dst_dir_full_name << " rename error: " << strerror(errno);

		return false;
	}
	return true;
}

bool DiskHelper::CopyDirectorys(vector<string>& src_dir_full_names, const string& dst_dir_full_name)
{
	for(vector<string>::iterator iter = src_dir_full_names.begin(); iter != src_dir_full_names.end(); iter++)    
	{
		if(!CopyDirectory(*iter, dst_dir_full_name))
		{
			return false;
		}
	}
	return true;
}

bool DiskHelper::RemoveDirectory(const string& dir_full_name)
{
	if(!DirectoryExist(dir_full_name))
		return false;

	if(remove(dir_full_name.c_str()) != 0)
	{
		GLERROR << dir_full_name << " remove error: " << strerror(errno);

		return false;
	}
	return true;

}

bool DiskHelper::RemoveDirectorys(vector<string>& dir_full_names)
{
	for(vector<string>::iterator iter = dir_full_names.begin(); iter != dir_full_names.end(); iter++)    
	{
		if(!RemoveDirectory(*iter))
		{
			return false;
		}
	}
	return true;
}

bool DiskHelper::EnumAllFiles(const string& dir_full_name, const string& filter, vector<string>& file_names)
{
	if(!DirectoryExist(dir_full_name))
		return false;

	DIR* dir = opendir(dir_full_name.c_str());
	if(dir == NULL)
	{
		GLERROR << dir_full_name << " opendir error: " << strerror(errno);
		closedir(dir); 
		return false;
	}

	struct dirent* de = NULL;
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_ino == 0 || de->d_name[0] == '.') 
			continue;

		string fileName = de->d_name;
		string fileFullName = dir_full_name + SPLITER + fileName;

		DIR* tmpDir = opendir(fileFullName.c_str());
		if(tmpDir !=  NULL)
		{
			closedir(tmpDir);
			continue;
		}

		if(!filter.empty())
		{
			if(string::npos == fileName.find(filter, 0))
				continue;
		}
		file_names.push_back(fileName);
	}
	closedir(dir);

	return true;
}

bool DiskHelper::RecurseEnumAllFiles(const string& dir_full_name, const string& filter, vector<string>& file_full_names)
{
	if(!DirectoryExist(dir_full_name))
		return false;

	DIR* dir = opendir(dir_full_name.c_str());
	if(dir == NULL)
	{
		GLERROR << dir_full_name << " opendir error: " << strerror(errno);
	
		closedir(dir); 
		return false;
	}

	struct dirent* de = NULL;
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_ino == 0 || de->d_name[0] == '.') 
			continue;

		string fileName = de->d_name;
		string fileFullName = dir_full_name + SPLITER + fileName;
		DIR* tmpDir = opendir(fileFullName.c_str());
		if(tmpDir !=  NULL)
		{
			closedir(tmpDir);
			RecurseEnumAllFiles(fileFullName, filter, file_full_names);
			continue;
		}

		if(!filter.empty()) 
		{
			if(string::npos == fileName.find(filter, 0))
				continue;
		}
		file_full_names.push_back(fileFullName);
	}
	closedir(dir);

	return true;
}

bool DiskHelper::EnumAllDirectory(const string& dir_full_name, const string& filter, vector<string>& dir_names)
{
	if(!DirectoryExist(dir_full_name))
		return false;

	DIR* dir = opendir(dir_full_name.c_str());
	if(dir == NULL)
	{
		GLERROR << dir_full_name << " opendir error: " << strerror(errno);

		closedir(dir); 
		return false;
	}

	struct dirent* de = NULL;
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_ino == 0 || de->d_name[0] == '.') 
			continue;

		string fileName = de->d_name;
		string fileFullName = dir_full_name + SPLITER + fileName;

		DIR* tmpDir = opendir(fileFullName.c_str());
		if(tmpDir !=  NULL)
		{
			closedir(tmpDir);
			if(!filter.empty()) 
			{
				if(string::npos == fileName.find(filter, 0))
					continue;
			}
			dir_names.push_back(fileName);
		}
	}
	closedir(dir); 
	return true;
}

bool DiskHelper::RecurseEnumAllDirectory(const string& dir_full_name, const string& filter, vector<string>& dir_full_names)
{
	if(!DirectoryExist(dir_full_name))
		return false;

	DIR* dir = opendir(dir_full_name.c_str());
	if(dir == NULL)
	{
		GLERROR << dir_full_name << " opendir error: " << strerror(errno);

		closedir(dir); 
		return false;
	}

	struct dirent* de = NULL;
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_ino == 0 || de->d_name[0] == '.') 
			continue;

		string fileName = de->d_name;
		string fileFullName = dir_full_name + SPLITER + fileName;

		DIR* tmpDir = opendir(fileFullName.c_str());
		if(tmpDir !=  NULL)
		{
			closedir(tmpDir);
			RecurseEnumAllDirectory(fileFullName, filter, dir_full_names);
			if(!filter.empty()) 
			{
				if(string::npos == fileName.find(filter, 0))
					continue;
			}
			dir_full_names.push_back(fileFullName);
		}
	}
	closedir(dir); 
	return true;
}
