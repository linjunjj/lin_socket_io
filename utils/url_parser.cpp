#include "url_parser.h"
#include <string.h>
#include "log/logger.h"

using namespace lin_io;

string UrlParser::Build()
{
	string url;
	url += _protocol;
	url += "://";
	url += _host;
	if(!_port.empty())
	{
		url += ":";
		url += _port;
	}
	url += "/";
	url += _path;
	url += "?";

	string flag;
	for(map<string,string>::iterator it = _params.begin(); it != _params.end(); it++)
	{
		url += flag;
		url += it->first;
		url += "=";
		url += it->second;
		flag = "&";
	}

	return EncodeURL(url);
}

bool UrlParser::Parse(const string& url)
{
	string data = DecodeURL(url);
	return Parse(data.c_str());
}

bool UrlParser::Parse(const char* url)
{
	GLDEBUG << url;
	const char* curstr = url;
	/*
	 * :
	 * := [a-z\+\-\.]+
	 * upper case = lower case for resiliency
	 */
	/* Read scheme */
	const char* tmpstr = strchr(curstr, ':');
	if ( NULL == tmpstr )
	{
		/* Not found the character */
		return false;
	}
	/* Get the scheme length */
	int len = tmpstr - curstr;
	/* Check restrictions */
	for (int i = 0; i < len; i++ )
	{
		if ( !IsSchemeChar(curstr[i]) )
		{
			/* Invalid format */
			GLWARN << "Invalid format, url: " << curstr;
			return false;
        }
    }
    /* Copy the scheme to the storage */
    _protocol.assign(curstr, len);

    /* Skip ':' */
    tmpstr++;
    curstr = tmpstr;
    /*
     * //:@:/
     * Any ":", "@" and "/" must be encoded.
     */
	/* Eat "//" */
	for (int i = 0; i < 2; i++ )
	{
		if ( '/' != *curstr )
		{
			GLWARN << "not find /, url: " << curstr;
			return false;
        }
        curstr++;
    }

	/* Check if the user (and password) are specified. */
	int userpass_flag = 0;
    tmpstr = curstr;
	while ( '\0' != *tmpstr )
	{
		if ( '@' == *tmpstr )
		{
			/* Username and password are specified */
            userpass_flag = 1;
            break;
        }
		else if ( '/' == *tmpstr )
		{
			/* End of : specification */
            userpass_flag = 0;
            break;
        }
        tmpstr++;
    }

	/* User and password specification */
    tmpstr = curstr;
    if ( userpass_flag )
    {
    	/* Read username */
    	while ( '\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr )
    	{
            tmpstr++;
        }
        len = tmpstr - curstr;
        _username.assign(curstr, len);

        /* Proceed current pointer */
        curstr = tmpstr;
        if ( ':' == *curstr )
        {
        	/* Skip ':' */
            curstr++;
            /* Read password */
            tmpstr = curstr;
            while ( '\0' != *tmpstr && '@' != *tmpstr )
            {
                tmpstr++;
            }
            len = tmpstr - curstr;
            _password.assign(curstr, len);
            curstr = tmpstr;
        }
        /* Skip '@' */
        if ( '@' != *curstr )
        {
        	GLWARN << "not find @, url: " << curstr;
        	return false;
        }
        curstr++;
    }

    int bracket_flag = 0;
    if ( '[' == *curstr )
    {
        bracket_flag = 1;
    }

    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    while ( '\0' != *tmpstr )
    {
    	if ( bracket_flag && ']' == *tmpstr )
    	{
    		/* End of IPv6 address. */
            tmpstr++;
            break;
        }
    	else if ( !bracket_flag && (':' == *tmpstr || '/' == *tmpstr) )
    	{
    		/* Port number is specified. */
    		break;
        }
        tmpstr++;
    }
    len = tmpstr - curstr;
    _host.assign(curstr, len);
    curstr = tmpstr;

    /* Is port number specified? */
    if ( ':' == *curstr )
    {
        curstr++;
       /* Read port number */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '/' != *tmpstr )
        {
            tmpstr++;
        }
        len = tmpstr - curstr;
        _port.assign(curstr, len);
        curstr = tmpstr;
    }

    /* End of the string */
    if ( '\0' == *curstr )
    {
    	return true;
    }

	/* Skip '/' */
	//if ( '/' != *curstr ) {
	//return false;
	//}
	//curstr++;

    /* Parse path */
    tmpstr = curstr;
    while ( '\0' != *tmpstr && '?' != *tmpstr )
    {
        tmpstr++;
    }
    len = tmpstr - curstr;

    _path.assign(curstr, len);
    curstr = tmpstr;

	/* Is query specified? */
	if ( '?' == *curstr )
	{
		/* Skip '?' */
        curstr++;
        /* Read query */
        tmpstr = curstr;
        while ( '\0' != *tmpstr )
        {
            if('&' != *tmpstr)
            {
            	tmpstr++;
            	continue;
            }
            len = tmpstr - curstr;
            string param;
            param.assign(curstr, len);

            int pos = param.find('=');
            if(pos != -1)
            {
            	string name = param.substr(0,pos);
            	string value = param.substr(pos+1);
            	_params[name] = value;
            }

            tmpstr++;
            curstr = tmpstr;
        }

        len = tmpstr - curstr;
		string param;
		param.assign(curstr, len);

		int pos = param.find('=');
		if(pos != -1)
		{
			string name = param.substr(0,pos);
			string value = param.substr(pos+1);
			_params[name] = value;
		}
    }

	return true;
}
