#ifndef __EZ_MAP_H_
#define __EZ_MAP_H_

#include <map>
#include <vector>
#include "mutex.h"

namespace lin_io
{
template<typename key_type, typename mapped_type>
class Map
{
public:
	typedef typename std::map<key_type, mapped_type>::iterator iterator;
	typedef typename std::map<key_type, mapped_type>::value_type value_type;

    Map();
    virtual ~Map();

	//返回插入个数
    int insert(const key_type&, const mapped_type&);

	//返回插入结果
	int insert(const key_type&, const mapped_type&, mapped_type&);

	//返回查找个数
    int find_by_key(const key_type&, mapped_type&);
    int find_by_value(const mapped_type&, std::vector<key_type>&);

	//返回删除个数
	int erase_by_key(const key_type&);
	int erase_by_value(const mapped_type&);

    int erase_by_key(const key_type&, mapped_type&);
	int erase_by_value(const mapped_type&, std::vector<key_type>&);

	//返回获取个数
	int get_all_keys(std::vector<key_type>&);
	int get_all_values(std::map<key_type, mapped_type>&);

	bool empty();
    void clear();
    void clear(std::map<key_type, mapped_type>&);
    unsigned int size();
	
protected:
	SpinLock _lock;
	std::map<key_type, mapped_type> _map;
};


template<typename key_type, typename mapped_type>
Map<key_type, mapped_type>::Map()
{
}


template<typename key_type, typename mapped_type>
Map<key_type, mapped_type>::~Map()
{
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::insert(const key_type& key, const mapped_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	std::pair<iterator, bool> ret = _map.insert(std::make_pair(key, value));
	return (ret.second ? 1 : 0);
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::insert(const key_type& key, const mapped_type& in, mapped_type& out)
{
	ScopLock<SpinLock> sync(_lock);
	std::pair<iterator, bool> ret = _map.insert(std::make_pair(key, in));
	if (ret.second)
	{
		out = in;
		return 1;
	}
	out = ret.first->second;

	return 0;
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::find_by_key(const key_type& key, mapped_type& value)
{
    ScopLock<SpinLock> sync(_lock);
    iterator it = _map.find(key);
    if(it != _map.end())
    {
        value = it->second;
        return 1;
    }

    return 0;
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::find_by_value(const mapped_type& value, std::vector<key_type>& keys)
{
    ScopLock<SpinLock> sync(_lock);
    for(iterator it = _map.begin(); it != _map.end(); it++)
    {
        if(it->second == value)
        {
            keys.push_back(it->first);
            continue;
        }
    }

	return (int)keys.size();
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::erase_by_key(const key_type& key)
{
	ScopLock<SpinLock> sync(_lock);
	iterator it = _map.find(key);
	if(it != _map.end())
	{
		_map.erase(it);
		return 1;
	}

	return 0;
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::erase_by_value(const mapped_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	int size(0);
	for(iterator it = _map.begin(); it !=_map.end(); it++)
	{
		if(it->second == value)
		{
			size++;
			_map.erase(it);
			continue;
		}
	}

	return size;
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::erase_by_key(const key_type& key, mapped_type& value)
{
    ScopLock<SpinLock> sync(_lock);
    iterator it = _map.find(key);
    if(it != _map.end())
    {
        value = it->second;
        _map.erase(it);
        return 1;
    }

    return 0;
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::erase_by_value(const mapped_type& value, std::vector<key_type>& keys)
{
    ScopLock<SpinLock> sync(_lock);
    for(iterator it = _map.begin(); it !=_map.end(); it++)
    {
        if(it->second == value)
        {
            keys.push_back(it->first);
            _map.erase(it);
            continue;
        }
    }

	return (int)keys.size();
}

template<typename key_type, typename mapped_type>
unsigned int Map<key_type, mapped_type>::size()
{
    ScopLock<SpinLock> sync(_lock);

    return (unsigned int)_map.size();
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::get_all_keys(std::vector<key_type>& keys)
{
    ScopLock<SpinLock> sync(_lock);
    for(iterator it = _map.begin(); it != _map.end(); it++)
    {
        keys.push_back(it->first);
    }

    return (int)keys.size();
}

template<typename key_type, typename mapped_type>
int Map<key_type, mapped_type>::get_all_values(std::map<key_type, mapped_type>& values)
{
    ScopLock<SpinLock> sync(_lock);
    for(iterator it = _map.begin(); it != _map.end(); it++)
    {
		values.insert(std::make_pair(it->first, it->second));
    }

    return (int)values.size();
}

template<typename key_type, typename mapped_type>
void Map<key_type, mapped_type>::clear()
{
    ScopLock<SpinLock> sync(_lock);
    _map.clear();
}

template<typename key_type, typename mapped_type>
void Map<key_type, mapped_type>::clear(std::map<key_type, mapped_type>& values)
{
    ScopLock<SpinLock> sync(_lock);
    for(iterator it = _map.begin(); it != _map.end(); it++)
    {
		values.insert(std::make_pair(it->first, it->second));
    }
    _map.clear();
}

template<typename key_type, typename mapped_type>
bool Map<key_type, mapped_type>::empty()
{
	ScopLock<SpinLock> sync(_lock);
	return _map.empty();
}

}

#endif

