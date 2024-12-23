#ifndef THREAD_LOCAL_H__
#define THREAD_LOCAL_H__

namespace com
{
template<class T>
class ThreadLocal
{
private:
    static thread_local T& Instance()
    {
        static thread_local T local;
        return local;
    }

public:
    static T& get()
    {
        return Instance();
    }
    static void set(const T& val)
    {
    	Instance() = val;
    }
};
}

#endif
