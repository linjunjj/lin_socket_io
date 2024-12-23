#ifndef __EASY_PUBLIC_H__
#define __EASY_PUBLIC_H__

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

namespace lin_io
{
inline
string toHexStr(const char* data,const int len)
{
	char* pStr = new char[len*3+1];
	for(int i = 0; i < len; i++)
	{
		sprintf(pStr+i*3,"%.2X ",(unsigned char)data[i]);
		//pStr[pStr+i*3 + 2] = ' ';
	}
	pStr[len*3] = 0;
	string hex(pStr);
	delete[] pStr;
	return hex;
}

inline
string toHexStr(const string data)
{
	return toHexStr(data.data(), (int)data.size());
}

inline
void Replace(string& data,string src, string dst)
{
	if (src == dst || src.empty())
		return;

	string::size_type pos = data.find(src);
	while(pos != std::string::npos)
	{
		data.replace(pos, src.size(), dst);
		pos = data.find(src, pos + dst.size());
	}
}

inline
string GetAppExePath()
{
	char buffer[256] = {0};
	int cnt = readlink("/proc/self/exe", buffer, 255);
	if(cnt > 0 && cnt <= 255)
	{
		return string(buffer);
	}
	return "";
}

inline
string GetAppDirPath()
{
	string path = GetAppExePath();
	int pos = path.rfind('/');
	if(pos != -1)
	{
		return path.substr(0,pos);
	}
	return path;
}

inline
time_t GetTotalSeconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

inline
time_t GetTotalMSeconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t ms = tv.tv_sec*1000 + tv.tv_usec/1000;
	return ms;
}

inline
time_t GetTotalUSeconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t us = tv.tv_sec*1000*1000 + tv.tv_usec;
	return us;
}

}

#endif
