#ifndef __EZ_ARGS_H_
#define __EZ_ARGS_H_

#include <map>
#include <string>
#include <memory>

namespace lin_io
{
class Args
{
	struct Val
	{
		Val() {}
		virtual ~Val() {}
		virtual void* getValue() const = 0;
	};

	template<typename TYPE>
	struct Arg : public Val
	{
		Arg(const TYPE& val):_val(new TYPE(val)) {}
		~Arg() {delete (TYPE*)_val;}
		virtual void* getValue() const {return _val;}
	private:
		void* _val;
	};

public:
	template<typename TYPE>
	void set(const std::string& name, const TYPE& value)
	{
		_args[name] = std::shared_ptr<Val>(new Arg<TYPE>(value));
	}

	template<typename TYPE>
	const TYPE& get(const std::string& name) const
	{
		std::map<std::string, std::shared_ptr<Val>>::const_iterator it = _args.find(name);
		if(it == _args.end())
			throw "not found arg";
		TYPE* val = (TYPE*)it->second->getValue();
		return *val;
	}

private:
	std::map<std::string, std::shared_ptr<Val>> _args;
};





}

#endif
