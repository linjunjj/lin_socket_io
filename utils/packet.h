#ifndef _SNOX_PACKET_H_INCLUDE__
#define _SNOX_PACKET_H_INCLUDE__

#include "int_types.h"

#include <string>
#include <iostream>
#include <stdexcept>
#include <map> // CARE
#include <set>
#include <vector>
#include <list>
#include "serializer.h"
#include "varstr.h"

namespace lin_io
{
struct string32 : public std::string
{
	virtual ~string32() {}
	string32():std::string() {}
	string32(const std::string &s):std::string(s) {}
	string32(const char *data, size_t size): std::string(data, size) {}
};

struct PacketError : public std::runtime_error
{
	PacketError(const std::string & w) :std::runtime_error(w)
	{
	}
	virtual ~PacketError() {}
};

struct PackError : public PacketError
{
	PackError(const std::string & w) :PacketError(w)
	{
	}
	virtual ~PackError() {}
};

struct UnpackError : public PacketError
{
	UnpackError(const std::string & w) :PacketError(w)
	{
	}
	virtual ~UnpackError() {}
};

//没啥用了
class PackBuffer
{
};

#if	defined(__i386__) || defined(__alpha__) \
		|| defined(__ia64) || defined(__ia64__) \
		|| defined(_M_IX86) || defined(_M_IA64) \
		|| defined(_M_ALPHA) || defined(__amd64) \
		|| defined(__amd64__) || defined(_M_AMD64) \
		|| defined(__x86_64) || defined(__x86_64__) \
		|| defined(_M_X64) || defined(WIN32) || defined(_WIN64)

#define XHTONS
#define XHTONL
#define XHTONLL

#else /* big end */

inline uint16_t XHTONS(uint16_t i16) {
	return ((i16 << 8) | (i16 >> 8));
}
inline uint32_t XHTONL(uint32_t i32) {
	return ((uint32_t(XHTONS(i32)) << 16) | XHTONS(i32>>16));
}
inline uint64_t XHTONLL(uint64_t i64) {
	return ((uint64_t(XHTONL((uint32_t)i64)) << 32) |XHTONL((uint32_t(i64>>32))));
}

#endif /* __i386__ */

#define XNTOHS XHTONS
#define XNTOHL XHTONL
#define XNTOHLL XHTONLL

class Pack : public Serializer
{
private:
	Pack (const Pack & o);
	Pack & operator = (const Pack& o);
public:
	Pack(int byteOrder = LITTLE_ENDIAN): Serializer(0,byteOrder){}

	Pack(PackBuffer& p)	{}
	virtual ~Pack() {}

	uint16_t xhtons(uint16_t i16) { return XHTONS(i16); }
	uint32_t xhtonl(uint32_t i32) { return XHTONL(i32); }
	uint64_t xhtonll(uint64_t i64) { return XHTONLL(i64); }

	Pack & push(const void * s, size_t n) { write(s,n); return *this; }
	Pack & push_uint8(uint8_t u8)	 { return push(&u8, 1); }
	Pack & push_uint16(uint16_t u16) { write_uint16(u16); return *this; }
	Pack & push_uint32(uint32_t u32) { write_uint32(u32); return *this; }
	Pack & push_uint64(uint64_t u64) { write_uint64(u64); return *this; }
	Pack & push_double(double dVal)  { write_double(dVal); return *this; }

	Pack & push_varstr(const Varstr & vs)     { return push_varstr(vs.data(), vs.size()); }
	Pack & push_varstr(const void * s)        { return push_varstr(s, strlen((const char *)s)); }
	Pack & push_varstr(const std::string & s) { return push_varstr(s.data(), s.size()); }
	Pack & push_varstr(const void * s, size_t len)
	{
		if (len > 0xFFFF) throw PackError("push_varstr: varstr too big");
		return push_uint16(uint16_t(len)).push(s, len);
	}
	Pack & push_varstr32(const std::string & s) { return push_varstr32(s.data(), s.size()); }
	Pack & push_varstr32(const void * s, size_t len)
	{
		if (len > 0xFFFFFFFF) throw PackError("push_varstr32: varstr too big");
		return push_uint32(uint32_t(len)).push(s, len);
	}
#ifdef WIN32
	Pack & push_varwstring32(const std::wstring &ws){
		size_t len = ws.size() * 2;
		return push_uint32((uint32_t)len).push(ws.data(), len);
	}
#endif
	void reserve(size_t n) {}
};

class Unpack : public Deserializer
{
public:
	uint16_t xntohs(uint16_t i16) const { return XNTOHS(i16); }
	uint32_t xntohl(uint32_t i32) const { return XNTOHL(i32); }
	uint64_t xntohll(uint64_t i64) const { return XNTOHLL(i64); }

	Unpack(const void * data = 0, size_t size = 0, int byteOrder = LITTLE_ENDIAN): Deserializer(data,size, byteOrder) {}
	virtual ~Unpack() {}
	std::string pop_varstr() const {
		Varstr vs = pop_varstr_ptr();
		return std::string(vs.data(), vs.size());
	}

	std::string pop_varstr32() const {
		Varstr vs = pop_varstr32_ptr();
		return std::string(vs.data(), vs.size());
	}
#ifdef WIN32
	std::wstring pop_varwstring32() const{
		Varstr vs = pop_varstr32_ptr();
		return std::wstring((wchar_t *)vs.data(), vs.size() / 2);
	}
#endif
	std::string pop_fetch(size_t k) const {
		return std::string(pop_fetch_ptr(k), k);
	}

	void finish() const {
		if (!empty()) throw UnpackError("finish: too much data");
	}

	uint8_t pop_uint8() const {
		return read_byte();
	}

	uint16_t pop_uint16() const {
		return read_uint16();
	}

	uint32_t pop_uint32() const {
		return read_uint32();
	}

	uint64_t pop_uint64() const {
		return read_uint64();
	}

	double pop_double() const {
		return read_double();
	}

	Varstr pop_varstr_ptr() const {
		// Varstr { uint16_t size; const char * data; }
		Varstr vs;
		vs.m_size = pop_uint16();
		vs.m_data = pop_fetch_ptr(vs.m_size);
		return vs;
	}

	Varstr pop_varstr32_ptr() const {
		Varstr vs;
		vs.m_size = pop_uint32();
		vs.m_data = pop_fetch_ptr(vs.m_size);
		return vs;
	}

	const char * pop_fetch_ptr(size_t k) const {
		if (size() < k)
		{
			//abort();
			throw UnpackError("pop_fetch_ptr: not enough data");
		}
		const char * p = data();
		skip(k);
		return p;
	}
};

//  interface Marshallable
struct Marshallable : public NioMarshallable
{
	virtual void marshal(Serializer&) const {};
	virtual void unmarshal(const Deserializer&) {};

	virtual void marshal(Pack&) const  = 0;
	virtual void unmarshal(const Unpack&) =0;

	virtual std::ostream & trace(std::ostream & os) const
	{
		return os << "trace Marshallable [ not immplement ]";
	}

	Marshallable()
	{
	}
	virtual ~Marshallable()
	{
	}
};

// Marshallable helper
inline std::ostream & operator << (std::ostream & os, Marshallable & m)
{
	return m.trace(os);
}

inline Pack & operator << (Pack & p, const Marshallable & m)
{
	m.marshal(p);
	return p;
}

inline const Unpack & operator >> (const Unpack & p, const Marshallable & m)
{
	const_cast<Marshallable &>(m).unmarshal(p);
	return p;
}

struct Voidmable : public Marshallable
{
	virtual ~Voidmable() {}
	virtual void marshal(Pack &) const {}
	virtual void unmarshal(const Unpack &) {}
};

struct Mulmable : public Marshallable
{
	virtual ~Mulmable() {}
	Mulmable(const Marshallable & m1, const Marshallable & m2)
				: mm1(m1), mm2(m2) { }

	const Marshallable & mm1;
	const Marshallable & mm2;

	virtual void marshal(Pack &p) const          { p << mm1 << mm2; }
	virtual void unmarshal(const Unpack &p) { assert(false); }
//	virtual std::ostream & trace(std::ostream & os) const { return os << mm1 << mm2; }
};

struct Mulumable : public Marshallable
{
	virtual ~Mulumable() {}
	Mulumable(Marshallable & m1, Marshallable & m2)
				: mm1(m1), mm2(m2) { }

	Marshallable & mm1;
	Marshallable & mm2;

	virtual void marshal(Pack &p) const          { p << mm1 << mm2; }
	virtual void unmarshal(const Unpack &p) { p >> mm1 >> mm2; }
	//virtual std::ostream & trace(std::ostream & os) const { return os << mm1 << mm2; }
};

struct Rawmable : public Marshallable
{
	virtual ~Rawmable() {}
	Rawmable(const char * data, size_t size) : m_data(data), m_size(size) {}

	template < class T >
	explicit Rawmable(T & t ) : m_data(t.data()), m_size(t.size()) { }

	const char * m_data;
	size_t m_size;

	virtual void marshal(Pack & p) const   { p.push(m_data, m_size); }
	virtual void unmarshal(const Unpack &) { assert(false); }
	//virtual std::ostream & trace(std::ostream & os) const { return os.write(m_data, m_size); }
};

// base type helper
inline Pack & operator << (Pack & p, bool sign)
{
	p.push_uint8(sign ? 1 : 0);
	return p;
}

inline Pack & operator << (Pack & p, uint8_t  u8)
{
	p.push_uint8(u8);
	return p;
}

inline Pack & operator << (Pack & p, int8_t  i8)
{
	p.push_uint8(i8);
	return p;
}

inline Pack & operator << (Pack & p, uint16_t  u16)
{
	p.push_uint16(u16);
	return p;
}

inline Pack & operator << (Pack & p, int16_t  i16)
{
	p.push_uint16(i16);
	return p;
}

inline Pack & operator << (Pack & p, uint32_t  u32)
{
	p.push_uint32(u32);
	return p;
}
inline Pack & operator << (Pack & p, uint64_t  u64)
{
	p.push_uint64(u64);
	return p;
}

inline Pack & operator << (Pack & p, int64_t  i64)
{
	p.push_uint64((uint64_t)i64);
	return p;
}

inline Pack & operator << (Pack & p, int32_t  i32)
{
	p.push_uint32((uint32_t)i32);
	return p;
}

inline Pack & operator << (Pack & p, double  dVal)
{
	p.push_double(dVal);
	return p;
}

inline Pack & operator << (Pack & p, const std::string & str)
{
	p.push_varstr(str);
	return p;
}

inline Pack & operator << (Pack & p, const string32 & str)
{
	p.push_varstr32(str.data(), str.size());
	return p;
}

#ifdef WIN32
inline Pack & operator << (Pack & p, const std::wstring & str)
{
	p.push_varwstring32(str);
	return p;
}
#endif
inline Pack & operator << (Pack & p, const Varstr & pstr)
{
	p.push_varstr(pstr);
	return p;
}

inline const Unpack & operator >> (const Unpack & p, Varstr & pstr)
{
	pstr = p.pop_varstr_ptr();
	return p;
}

// pair.first helper
// XXX std::map::value_type::first_type unpack 需要特别定义
inline const Unpack & operator >> (const Unpack & p, uint32_t & u32)
{
	u32 =  p.pop_uint32();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, uint64_t & u64)
{
	u64 =  p.pop_uint64();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, int64_t & i64)
{
	i64 =  (int64_t)p.pop_uint64();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, int32_t & i32)
{
	i32 =  (int32_t)p.pop_uint32();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, std::string & str)
{
	// XXX map::value_type::first_type
	str = p.pop_varstr();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, string32 & str)
{
	std::string s = p.pop_varstr32();
	str.swap(s);
	return p;
}

#ifdef WIN32
inline const Unpack & operator >> (const Unpack & p, std::wstring & str)
{
	// XXX map::value_type::first_type
	str = p.pop_varwstring32();
	return p;
}
#endif
inline const Unpack & operator >> (const Unpack & p, uint8_t & u8)
{
	u8 =  p.pop_uint8();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, int8_t & i8)
{
	i8 =  p.pop_uint8();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, uint16_t & u16)
{
	u16 =  p.pop_uint16();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, int16_t & i16)
{
	i16 =  p.pop_uint16();
	return p;
}

inline const Unpack & operator >> (const Unpack & p, bool & sign)
{
	sign =  (p.pop_uint8() == 0) ? false : true;
	return p;
}

inline const Unpack & operator >> (const Unpack & p, double & dVal)
{
	dVal =  p.pop_double();
	return p;
}

template <class T1, class T2>
inline std::ostream& operator << (std::ostream& s, const std::pair<T1, T2>& p)
{
	s << p.first << '=' << p.second;
	return s;
}

template <class T1, class T2>
inline Pack& operator << (Pack& s, const std::pair<T1, T2>& p)
{
	s << p.first << p.second;
	return s;
}

template <class T1, class T2>
inline const Unpack& operator >> (const Unpack& s, std::pair<const T1, T2>& p)
{
	const T1& m = p.first;
	T1 & m2 = const_cast<T1 &>(m);
	s >> m2 >> p.second;
	return s;
}

template <class T1, class T2>
inline const Unpack& operator >> (const Unpack& s, std::pair<T1, T2>& p)
{
	s >> p.first >> p.second;
	return s;
}

template <class T>
inline Pack& operator << (Pack& p, const std::set<T>& set)
{
	marshal_container(p, set);
	return p;
}

template <class T>
inline Pack& operator << (Pack& p, const std::vector<T>& cc)
{
	marshal_container(p, cc);
	return p;
}

template <class T1, class T2>
inline Pack& operator << (Pack& p, const std::map<T1, T2>& cc)
{
	marshal_container(p, cc);
	return p;
}

template <class T>
inline Pack& operator << (Pack& p, const std::list<T>& cc)
{
	marshal_container(p, cc);
	return p;
}

template <class T1, class T2>
inline Pack& operator << (Pack& p, const std::list<std::map<T1, T2> >& cc)
{
	marshal_composite_container(p, cc);
	return p;
}


template <class T>
inline const Unpack& operator >> (const Unpack& p, std::set<T>& set)
{
	unmarshal_container(p, std::inserter(set, set.begin()));
	return p;
}

template <class T>
inline const Unpack& operator >> (const Unpack& p, std::vector<T>& cc)
{
	unmarshal_container(p, std::back_inserter(cc));
	return p;
}

template <class T>
inline const Unpack& operator >> (const Unpack& p, std::list<T>& cc)
{
	unmarshal_container(p, std::back_inserter(cc));
	return p;
}

template <class T1, class T2>
inline const Unpack& operator >> (const Unpack& p, std::map<T1, T2>& cc)
{
	unmarshal_container(p, std::inserter(cc, cc.begin()));
	return p;
}

template <class T1, class T2>
inline const Unpack& operator >> (const Unpack& p, std::list<std::map<T1, T2> >& cc)
{
	unmarshal_composite_container(p, std::back_inserter(cc));
	return p;
}
// container marshal helper
template < typename ContainerClass >
inline void marshal_container(Pack & p, const ContainerClass & c)
{
	p.push_uint32(uint32_t(c.size())); // use uint32 ...
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		p << *i;
}


template < typename OutputIterator >
inline void unmarshal_container(const Unpack & p, OutputIterator i)
{
	for (uint32_t count = p.pop_uint32(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type tmp;
		p >> tmp;
		*i = tmp;
		++i;
	}
}

//add by heiway 2005-08-08
//and it could unmarshal list,vector etc..
template < typename OutputContainer>
inline void unmarshal_containerEx(const Unpack & p, OutputContainer & c)
{
	for(uint32_t count = p.pop_uint32() ; count >0 ; --count)
	{
		typename OutputContainer::value_type tmp;
		tmp.unmarshal(p);
		c.push_back(tmp);
	}
}

template < typename ContainerClass >
inline std::ostream & trace_container(std::ostream & os, const ContainerClass & c, char div='\n')
{
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		os << *i << div;
	return os;
}

// marshal container like this: list< map<key_type, value_ype> >
// container marshal helper
template < typename ContainerClass >
inline void marshal_composite_container(Pack & p, const ContainerClass & c)
{
	p.push_uint32(uint32_t(c.size())); // use uint32 ...
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		marshal_container(p, *i);
}

// unmarshal container like this: list< vector<value_ype> >
template < typename OutputIterator >
inline void unmarshal_composite_array(const Unpack & up, OutputIterator i)
{
	for (uint32_t count = up.pop_uint32(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type tmp;
		unmarshal_container(up, std::back_inserter(tmp));
		*i = tmp;
		++i;
	}
}

// unmarshal container like this: list< map<key_type, value_ype> >
template < typename OutputIterator >
inline void unmarshal_composite_container(const Unpack & up, OutputIterator i)
{
	for (uint32_t count = up.pop_uint32(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type tmp;
		unmarshal_container(up, std::inserter(tmp, tmp.begin()));
		*i = tmp;
		++i;
	}
}

// marshal map like this: map<key_type, map<key_type, value_ype> >
// container marshal helper
template < typename ContainerClass >
inline void marshal_composite_map(Pack & p, const ContainerClass & c)
{
	p.push_uint32(uint32_t(c.size())); // use uint32 ...
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
	{	p << i->first;
	marshal_container(p, i->second);
	}
}

// unmarshal container like this: map<key_type, map<key_type, value_ype> >
template < typename OutputIterator >
inline void unmarshal_composite_map(const Unpack & up, OutputIterator i)
{
	for (uint32_t count = up.pop_uint32(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type tmp;
		const typename OutputIterator::container_type::key_type& m = tmp.first;
		typename OutputIterator::container_type::key_type & m2 = const_cast<typename OutputIterator::container_type::key_type &>(m);
		up >> m2;
		unmarshal_container(up, std::inserter(tmp.second, tmp.second.begin()));
		*i = tmp;
		++i;
	}
}

inline void PacketToString(const Marshallable &objIn, std::string &strOut)
{
	Pack pack;
	objIn.marshal(pack);
	strOut.assign(pack.data(), pack.size());
}

inline bool StringToPacket(const std::string &strIn, Marshallable &objOut)
{
	try
	{
		Unpack unpack(strIn.data(), strIn.length());
		objOut.unmarshal(unpack);
	}
	catch (UnpackError& e)
	{
		return false;
	}
	return true;
}

} // namespace

#endif // _SNOX_PACKET_H_INCLUDE__
