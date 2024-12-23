/********************************************************************************************
 * file name:   disk_helper
 * creator:     wsl
 * create date: 2014-02-26
 * support:     QQ[355006965]
 * update describe:
 ********************************************************************************************/

#ifndef __DISK_HELPER_H
#define __DISK_HELPER_H

#include <string>
#include <vector>

#define SPLITER "/"
#define POWER 0755 

namespace lin_io
{
using namespace std;
struct FileAttribute
{
	string m_fileName;
	string m_fileDir;
	size_t m_fileSize;
	time_t m_lastModifyTime;

	FileAttribute();
	~FileAttribute();
	void Reset();
};

class DiskHelper
{
public:
	DiskHelper();
	virtual ~DiskHelper();

	bool FileExist(const string& file_full_name);
	bool FileInformation(const string& file_full_name, FileAttribute& file_attr);

	bool OpenFile(const string& file_full_name, string& file_data);
	bool CreateFile(const string& file_full_name,const char* file_mode = "w",const char* file_data = NULL,const int data_len = 0);

	bool CopyFile(const string& src_file_full_name, const string& dst_file_full_name);
	bool CopyFiles(vector<string>& src_file_full_names, const string& dst_dir_full_name);
	bool RemoveFile(const string& file_full_name);
	bool RemoveFiles(vector<string>& file_full_names);

	bool DirectoryExist(const string& dir_full_name);
	bool CreateDirectory(const string& dir_full_name,const bool is_dir_tree = false);
	bool CopyDirectory(const string& src_dir_full_name, const string& dst_dir_full_name);
	bool CopyDirectorys(vector<string>& src_dir_full_names, const string& dst_dir_full_name);
	bool RemoveDirectory(const string& dir_full_name);
	bool RemoveDirectorys(vector<string>& dir_full_names);

	bool EnumAllFiles(const string& dir_full_name, const string& filter, vector<string>& file_names);
	bool EnumAllDirectory(const string& dir_full_name, const string& filter, vector<string>& dir_names);
	bool RecurseEnumAllFiles(const string& dir_full_name, const string& filter, vector<string>& file_full_names);
	bool RecurseEnumAllDirectory(const string& dir_full_name, const string& filter, vector<string>& dir_full_names);
};
}
#endif
