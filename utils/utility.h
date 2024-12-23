#ifndef		__NIO_UTILS_H__
#define		__NIO_UTILS_H__

#include 	<stdio.h>
#include 	<assert.h>
#include	<string.h>
#include	<stdarg.h>
#include 	<pthread.h>

#include	<string>
#include	<map>
#include 	<vector>
#include 	<set>
#include 	<sys/time.h>
#include 	<stdint.h>
#include 	<stdlib.h>
#include 	<algorithm>
#include 	<sstream>
#include    <ctype.h>

#define	throw_exceptions

namespace lin_io
{
inline
bool isNumber(const std::string& str)
{
	for(size_t i = 0; i < (size_t)str.size(); i++)
	{
		char c = str[i];
		if(!isdigit(c))
		{
			return false;
		}
	}
	return true;
}

inline
uint32_t atou32(const char *nptr)
{
	return strtoul(nptr,0,10);
}

using namespace std;
template<size_t N>
class   Format{
	char    buf_[N] ;
public:
	Format(){}
	const char * operator()(const char * fmt,...){
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf_,sizeof(buf_),fmt,args) ;
		va_end(args) ;
		return (const char *)buf_ ;
	}
	const char * format(const char * fmt,...){
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf_,sizeof(buf_),fmt,args) ;
		va_end(args) ;
		return (const char *)buf_ ;
	}
};
typedef Format<1024>       Format1k ;
typedef Format<2*1024>     Format2k ;

inline
std::string		u322s(uint32_t v) {
	char buffer[64];
	sprintf(buffer,"%u", v);
	return std::string(buffer);
}
inline
std::string		u642s(uint64_t v) {
	return std::to_string(v);
}

//  ip is net bitorder!
inline
std::string	  ip4n2a(uint32_t ip) {
	unsigned char * p =   reinterpret_cast<unsigned char *>(&ip) ;
	char	buf[16] ;
	sprintf(buf,"%u.%u.%u.%u",(uint32_t)p[0],(uint32_t)p[1],(uint32_t)p[2],(uint32_t)p[3]) ;
	return std::string((const char *)buf) ;
}

inline
void split2array(const char * str,const char * seperator,std::vector<std::string> & arr){
	assert(str) ;
	assert(seperator) ;

	size_t	n  = strlen(seperator) ;
	const char * pos = strstr(str,seperator) ;
	while( pos ){
		arr.push_back(std::string(str,pos-str)) ;
		str = pos + n ;
		pos = strstr(str,seperator) ;
	}
	arr.push_back(std::string(str)) ;
}

//	return consume bytes
inline
size_t getline(const char * s,size_t n,size_t i,std::string & line){
	if( i >= n )	return 0 ;
	const char * p = strchr(s,'\n') ;
	if( !p ){
		p = s+n ;
		i = p - s ;
	}else{
		i = p+1-s ;
	}
	while( p > s && ( *(p-1)=='\r' ) ) -- p ;
	line.assign(s,p-s) ;
	return i ;
}


//	return consume bytes
inline
size_t getline(const std::string & str,size_t i,std::string & line){
	return getline(str.data(),str.length(),i,line) ;
}

inline
void lines(const std::string & str,std::vector<std::string> & lines){
	size_t n = str.length() ;
	size_t i = 0 ;
	do{
		std::string line ;
		i += getline(str,i,line) ;
		lines.push_back(line) ;
	}while( i<n ) ;
}

inline
void tokenize(std::vector<std::string> & out,const std::string & str, const std::string & delims=", \t"){
	using namespace std;
	string::size_type lastPos = str.find_first_not_of(delims, 0);
	string::size_type pos     = str.find_first_of(delims, lastPos);

	while (string::npos != pos || string::npos != lastPos){
		// Found a token, add it to the vector.
		out.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delims.  Note the "not_of". this is beginning of token
		lastPos = str.find_first_not_of(delims, pos);
		// Find next delimiter at end of token.
		pos     = str.find_first_of(delims, lastPos);
	}
}

inline vector<std::string> tokenize_str(const std::string & str,const std::string & delims)
					{
	// Skip delims at beginning, find start of first token
	string::size_type lastPos = str.find_first_not_of(delims, 0);
	// Find next delimiter @ end of token
	string::size_type pos     = str.find_first_of(delims, lastPos);

	// output vector
	vector<string> tokens;

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delims.  Note the "not_of". this is beginning of token
		lastPos = str.find_first_not_of(delims, pos);
		// Find next delimiter at end of token.
		pos     = str.find_first_of(delims, lastPos);
	}

	return tokens;
					}


inline vector<std::string> tokenize_str(const std::string & str, const vector<std::string> & delims)
{
	vector<std::string> result = { str };
	vector<std::string> temp;
	for (auto& del : delims)
	{
		for (auto& res : result)
		{
			vector<std::string> vRes = tokenize_str(res, del);
			temp.insert(temp.end(), vRes.begin(), vRes.end());
		}
		result.swap(temp);
		temp.resize(0);
	}
	return result;
}

inline
int64_t nowUs()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return static_cast<int64_t>(tv.tv_sec)*1000000 + tv.tv_usec;
}
inline
uint32_t atoul(const char *nptr)
{
	return strtoul(nptr,0,10);
}

inline
uint64_t atoull(const char *nptr)
{
	return strtoull(nptr,0,10);
}

inline
uint64_t atou64(const char *nptr)
{
	return strtoull(nptr,0,10);
}
inline
void toLower(std::string& cmd)
{
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
}
inline
void toUpper(std::string& cmd)
{
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
}

inline void str2hex(const char* data, size_t len, char **outbuf) {
	if (data == 0){outbuf = 0;return;}
	char *buf = new char [len*2+1];
	static char dict[16]= {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	int j =0;
	for (size_t i = 0;i<len;i++) {
		unsigned char c  = data[i];
		int h = c/16;
		int l = c%16;
		buf[j++]=dict[h];
		buf[j++]=dict[l];
	}
	buf[j] = 0;
	*outbuf = buf;
}

template<class Key, class Val>
inline void DiffNewOldMap(const map<Key, Val>& newMap, const map<Key, Val>& oldMap, vector<pair<Key,Val>>& add, vector<pair<Key,Val>>& rem)
{
	auto max = newMap.size() > oldMap.size() ? newMap.size() : oldMap.size();
	auto newIt = newMap.begin();
    auto oldIt = oldMap.begin();
    for (int i=0; i < max; ++i)
    {
        //判断是否循环退出
        if (newIt == newMap.end())
        {
            for (auto it = oldIt; it != oldMap.end(); ++it)
            {
                rem.push_back(*it);
            }
            break;
        }
        if (oldIt == oldMap.end())
        {
            for (auto it = newIt; it != newMap.end(); ++it)
            {
                add.push_back(*it);
            }
            break;
        }

        if (newIt->first == oldIt->first)
        {
            ++newIt;
            ++oldIt;
        }
        else if (newIt->first > oldIt->first)
        {
            rem.push_back(*oldIt);
            ++oldIt;
        }
        else
        {
            add.push_back(*newIt);
            ++newIt;
        }
    }
}

inline uint32_t getHashKey(const string& key)
{
    const char* ckey = key.c_str();
    long hash = 5381;
    for(unsigned int i = 0; i < key.length(); i++)
    {
	    hash = ((hash << 5) + hash) + ckey[i];
	    hash = hash & 0xFFFFFFFFl;
    }
    return hash;
}

}

#endif	//	__NIO_UTILS_H__
