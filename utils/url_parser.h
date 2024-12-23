#ifndef __EASY_PARSE_URL_H__
#define __EASY_PARSE_URL_H__

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

namespace lin_io
{
using namespace std;

inline
unsigned char CharToHex( unsigned char x )
{
	return (unsigned char)(x > 9 ? x + 55 : x + 48);
}

inline
int IsAlphaNumberChar( unsigned char c )
{
	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
	  return 1;
	return 0;
}

//url编码实现
inline
string EncodeURL(const string& src )
{
	string dst;
	for (uint32_t i = 0; i < src.size(); i++)
	{
	  unsigned char ch = (unsigned char)src[i];
	  if (ch == ' ')
	  {
		  dst += "%20";
	  }
	  else if (IsAlphaNumberChar(ch) || strchr("-_.!~*'()", ch))
	  {
		  dst += ch;
	  }
	  else
	  {
		  dst += '%';
		  dst += CharToHex( (unsigned char)(ch >> 4) );
		  dst += CharToHex( (unsigned char)(ch % 16) );
	  }
	}
	return dst;
}

//解url编码实现
inline
string DecodeURL(const string& src)
{
	string dst;
    for (uint32_t i = 0; i < src.size(); i++)
    {
        if( src[i] != '%' )
        {
        	dst += src[i];
            continue;
        }

        char p[2] = {0};
        p[0] = src[++i];
        p[1] = src[++i];
        p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);
        p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);
        dst += (unsigned char)(p[0] * 16 + p[1]);
    }
    return dst;
}

class UrlParser
{
public:
    const string GetProtocol() const
    {
    	return _protocol;
    }
    const string GetHost() const
    {
    	return _host;
    }
    const string GetPort() const
    {
    	return _port;
    }
    const string GetPath() const
    {
    	return _path;
    }
    const string GetParam(const string& key)
    {
    	map<string,string>::iterator it = _params.find(key);
    	if(it != _params.end())
    		return it->second;
    	return "";
    }
    const string GetUserName() const
    {
    	return _username;
    }
    const string GetPassword() const
    {
    	return _password;
    }

    void SetProtocol(const string& protocol)
    {
         _protocol = protocol;
    }
    void SetHost(const string& host)
    {
        _host = host;
    }
    void SetPort(const string& port)
    {
        _port = port;
    }
    void SetPath(const string& path)
    {
        _path = path;
    }
    void SetParam(const string& key,const string& val)
    {
        _params[key] = val;
    }

    bool Parse(const char* url);
    bool Parse(const string& url);
    string Build();
private:
    bool IsSchemeChar(int c)
    {
    	return (!isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
    }
private:
	string _protocol;             /* mandatory */
	string _host;                 /* mandatory */
	string _port;                 /* optional */
	string _path;                 /* optional */
	string _username;             /* optional */
	string _password;             /* optional */
	map<string,string> _params;
};
}

#endif//__URL_H__
