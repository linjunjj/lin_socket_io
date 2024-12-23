#include "date_time.h"
#include <iostream>
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;
using namespace lin_io;

DateTime::DateTime()
{
	struct timeval now_timeval;
	gettimeofday(&now_timeval,0);

#ifdef HPUX
	struct tm* fast_tm = NULL;
	fast_tm = localtime((time_t*)&now_timeval.tv_sec);
	if(fast_tm == NULL)
        return;

	this->m_year     =  fast_tm->tm_year+1900;
	this->m_month    =  fast_tm->tm_mon+1;
	this->m_mday     =  fast_tm->tm_mday;
	this->m_wday	 =  fast_tm->tm_wday;
	this->m_hours    =  fast_tm->tm_hour;
	this->m_minutes  =  fast_tm->tm_min;
	this->m_seconds  =  fast_tm->tm_sec;  
	//this->m_microseconds  = now_timeval.tv_usec;
#else
	struct tm fast_tm;
	localtime_r((time_t*)&now_timeval.tv_sec, &fast_tm);
	this->m_year     =  fast_tm.tm_year+1900;
	this->m_month    =  fast_tm.tm_mon+1;
	this->m_mday     =  fast_tm.tm_mday;
	this->m_wday	 =  fast_tm.tm_wday;
	this->m_hours    =  fast_tm.tm_hour;
	this->m_minutes  =  fast_tm.tm_min;
	this->m_seconds  =  fast_tm.tm_sec;  
	//this->m_microseconds  = now_timeval.tv_usec;
#endif
}

DateTime::DateTime(const std::string& time, std::string format)
{
	size_t pos = format.find("%Y", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%04d");

	pos = format.find("%m", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%02d");

	pos = format.find("%d", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%02d");

	pos = format.find("%H", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%02d");

	pos = format.find("%M", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%02d");

	pos = format.find("%S", 0);
	if(pos == std::string::npos)
		return;
	format.replace(pos, 2, "%02d");

	//std::cout << format << endl;
	sscanf(time.c_str(),format.c_str(),&m_year,&m_month,&m_mday,&m_hours,&m_minutes,&m_seconds);

	SetTime(GetTimeS());
}

std::string DateTime::FormatTime(std::string format)
{
	size_t pos = format.find("%Y", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%04d",m_year);
		format.replace(pos, 2, result);
	}

	pos = format.find("%m", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%02d",m_month);
		format.replace(pos, 2, result);
	}

	pos = format.find("%d", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%02d",m_mday);
		format.replace(pos, 2, result);
	}

	pos = format.find("%H", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%02d",m_hours);
		format.replace(pos, 2, result);
	}

	pos = format.find("%M", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%02d",m_minutes);
		format.replace(pos, 2, result);
	}

	pos = format.find("%S", 0);
	if(pos != std::string::npos)
	{
		char result[30];
		memset(result,0,30);
		snprintf(result, sizeof(result),"%02d",m_seconds);
		format.replace(pos, 2, result);
	}
	return format;
}

DateTime::DateTime(const int year, const int month, const int day, const int hrs, const int min, const int sec)
	:m_year(year),m_month(month),m_mday(day),m_hours(hrs),m_minutes(min),m_seconds(sec)
{
	SetTime(GetTimeS());
	//this->m_microseconds=0;  
}

DateTime::DateTime(time_t time)
{
	SetTime(time);
	//this->m_microseconds=0;  
}

DateTime::DateTime(const DateTime& rhs)
{
	this->m_year     =  rhs.m_year; 
	this->m_month    =  rhs.m_month;
	this->m_mday     =  rhs.m_mday;
	this->m_wday     =  rhs.m_wday;
	this->m_hours    =  rhs.m_hours;
	this->m_minutes  =  rhs.m_minutes;
	this->m_seconds  =  rhs.m_seconds;
	//this->m_microseconds = rhs.m_microseconds;
}

DateTime& DateTime::operator =(const DateTime& rhs)
{
    m_year     =  rhs.m_year;
	m_month    =  rhs.m_month;
	m_mday     =  rhs.m_mday;
	m_wday     =  rhs.m_wday;
	m_hours    =  rhs.m_hours;
	m_minutes  =  rhs.m_minutes; 
	m_seconds  =  rhs.m_seconds;
	//m_microseconds  = rhs.m_microseconds;   
    return *this;
}

time_t DateTime::GetTimeS()
{
	struct tm fast_tm;
	fast_tm.tm_year=m_year-1900;
	fast_tm.tm_mon=m_month-1;
	fast_tm.tm_mday=m_mday;
	fast_tm.tm_hour=m_hours;
	fast_tm.tm_min=m_minutes;
	fast_tm.tm_sec=m_seconds;
	fast_tm.tm_isdst = -1;
	time_t now_seconds=mktime(&fast_tm);
	return now_seconds;
}

void DateTime::SetTime(time_t time)
{
#ifdef HPUX
	struct tm* fast_tm = NULL;
	fast_tm = localtime((time_t*)&time);
	if(fast_tm == NULL)
		return;
	this->m_year     =  fast_tm->tm_year+1900;
	this->m_month    =  fast_tm->tm_mon+1;
	this->m_mday     =  fast_tm->tm_mday;
	this->m_wday     =  fast_tm->tm_wday;
	this->m_hours    =  fast_tm->tm_hour;
	this->m_minutes  =  fast_tm->tm_min;
	this->m_seconds  =  fast_tm->tm_sec; 
#else
	struct tm fast_tm;
	localtime_r((time_t*)&time, &fast_tm);
	this->m_year     =  fast_tm.tm_year+1900;
	this->m_month    =  fast_tm.tm_mon+1;
	this->m_mday     =  fast_tm.tm_mday;
	this->m_wday	 =  fast_tm.tm_wday;
	this->m_hours    =  fast_tm.tm_hour;
	this->m_minutes  =  fast_tm.tm_min;
	this->m_seconds  =  fast_tm.tm_sec;  
#endif
}

int DateTime::GetYear()
{
	return m_year;
}

void DateTime::SetYear(int year)
{
	m_year = year;
}

int DateTime::GetMonth()
{
	return m_month;
}

void DateTime::SetMonth(int month)
{
	m_month=month;
}

int DateTime::GetMDay()
{
	return m_mday;
}

void DateTime::SetMDay(int day)
{
	m_mday = day;
}

int DateTime::GetWDay()
{
	return m_wday;
}

void DateTime::SetWDay(int day)
{
	m_wday = day;
}

int DateTime::GetHours()
{
	return m_hours;
}

void DateTime::SetHours(int hours)
{
	m_hours = hours;
}

int DateTime::GetMinutes()
{
	return m_minutes;
}

void DateTime::SetMinutes(int minutes)
{
	m_minutes = minutes;
}

int DateTime::GetSeconds()
{
	return m_seconds;
}

void DateTime::SetSeconds(int seconds) 
{
	m_seconds = seconds;
}

