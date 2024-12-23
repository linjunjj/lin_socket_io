/********************************************************************************************
 * file name:   DateTime.h
 * creator:     wsl
 * create date: 2014-02-14
 * support:     QQ[355006965] E-mail[wangsl3@asiainfo-linkage.com]
 * update describe:
 ********************************************************************************************/

#ifndef __EASY_DATETIME_H__
#define __EASY_DATETIME_H__
#include <string>

namespace lin_io
{
class DateTime
{
public:
	DateTime(); 
	DateTime(const time_t time);
	DateTime(const int year, const int month, const int day, const int hrs=0, const int min=0, const int sec=0);
	DateTime(const std::string& time, std::string format="%Y-%m-%d %H:%M:%S");//'%Y%m%d%H%M%S'
	virtual ~DateTime() {}
public:
	DateTime(const DateTime& rhs);
    DateTime& operator =(const DateTime& rhs);

public:
	void SetTime(time_t time);
    time_t GetTimeS();
	std::string FormatTime(std::string format="%Y-%m-%d %H:%M:%S");//'%Y%m%d%H%M%S'

public:
	int GetYear();
	void SetYear(int year);

	int GetMonth();
	void SetMonth(int month);

	int GetMDay();
	void SetMDay(int day);

	int GetWDay();
	void SetWDay(int day);

	int GetHours();
	void SetHours(int hours);

	int GetMinutes();
	void SetMinutes(int minutes);

	int GetSeconds();
	void SetSeconds(int seconds); 
private:
	int m_year; 
	int m_month;
	int m_mday;
	int m_wday;
	int m_hours;
	int m_minutes;
	int m_seconds;
	//time_t m_microseconds;
};

}
#endif
