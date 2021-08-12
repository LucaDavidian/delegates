#ifndef SIGNAL_H
#define SIGNAL_H

#include "delegate.hpp"
#define TYPE_PARAM_HEAP_QUEUE
#include "../../data structures/ADT/priority queue/priority_queue.hpp"
#include <type_traits>

/***** signal typedefs *****/
#define SIGNAL(SignalType)                         typedef Signal<void()> SignalType
#define SIGNAL_ONE_PARAM(SignalType, par0)         typedef Signal<void(par0)> SignalType
#define SIGNAL_TWO_PARAM(SignalType, par0, par1)   typedef Signal<void(par0, par1)> SignalType

#define SIGNAL_RET(SignalType, ret)                         typedef Signal<ret()> SignalType
#define SIGNAL_RET_ONE_PARAM(SignalType, ret, par0)         typedef Signal<ret(par0)> SignalType
#define SIGNAL_RET_TWO_PARAM(SignalType, ret, par0, par1)   typedef Signal<ret(par0, par1)> SignalType

/**** signal primary class template (not defined) ****/
template <typename Signature>
class Signal;

/**** signal partial class template specialization for function types ****/
template <typename Ret, typename... Args>
class Signal<Ret(Args...)>  
{
friend class Connection;
public:
    // template <typename T, typename... Payload>
    // Connection Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority, Payload&&... payload);

    // template <typename T, typename... Payload>
    // Connection Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority, Payload&&... payload);

    template <typename T, typename PtrToMemFun, typename... Payload>
    std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>, Connection> Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority, Payload&&... payload);

    template <typename T, typename... Payload>
    Connection Bind(T &&funObj, unsigned int priority, Payload&&... payload);

    explicit operator bool() const { return !mDelegates.empty(); }

    void operator()(Args... args); 
    
    void Invoke(Args... args);

    template <typename F>
    void operator()(F const &f, Args... args);

    template <typename F>
    void Invoke(const F &f, Args... args);
private:
    void Unbind(CallableWrapper<Ret(Args...)> *callableWrapper);

    PriorityQueue<Delegate<Ret(Args...)>> mDelegates;
};

// template <typename Ret, typename... Args>
// template <typename T, typename... Payload>
// Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority, Payload&&... payload)
// {   
//     Delegate<Ret(Args...)> delegate;
//     delegate.Bind(instance, ptrToMemFun, priority, std::forward<Payload>(payload)...);
//     CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
//     mDelegates.Insert(std::move(delegate));

//     return Connection(this, callable); 
// }

// template <typename Ret, typename... Args>
// template <typename T, typename... Payload>
// Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority, Payload&&... payload)
// {
//     Delegate<Ret(Args...)> delegate;
//     delegate.Bind(instance, ptrToConstMemFun, priority, std::forward<Payload>(payload)...);
//     CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
//     mDelegates.Insert(std::move(delegate));

//     return Connection(this, callable); 
// }

template <typename Ret, typename... Args>
template <typename T, typename PtrToMemFun, typename... Payload>
std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>, Connection> Signal<Ret(Args...)>::Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority, Payload&&... payload)
{
    Delegate<Ret(Args...)> delegate;
    delegate.Bind(instance, ptrToMemFun, priority, std::forward<Payload>(payload)...);
    CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
    mDelegates.Insert(std::move(delegate));

    return Connection(this, callable); 
}
    
template <typename Ret, typename... Args>
template <typename T, typename... Payload>
Connection Signal<Ret(Args...)>::Bind(T &&funObj, unsigned int priority, Payload&&... payload)
{
    Delegate<Ret(Args...)> delegate;
    delegate.Bind(std::forward<T>(funObj), priority, std::forward<Payload>(payload)...);
    CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
    mDelegates.Insert(std::move(delegate));

    return Connection(this, callable);  
}

template <typename Ret, typename... Args>
void Signal<Ret(Args...)>::Unbind(CallableWrapper<Ret(Args...)> *callableWrapper)
{
    PriorityQueue<Delegate<Ret(Args...)>> queue;

    while (!mDelegates.Empty())
    {
        if (mDelegates.Peek().mCallableWrapper == callableWrapper)
            mDelegates.Remove();
        else
        {
            queue.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(mDelegates.Peek())));
            mDelegates.Remove();
        }
    }

    while (!queue.Empty())
    {
        mDelegates.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(queue.Peek())));
        queue.Remove();
    }
}

template <typename Ret, typename... Args>
void Signal<Ret(Args...)>::operator()(Args... args) 
{
    Invoke(std::forward<Args>(args)...);
}
    
template <typename Ret, typename... Args>
void Signal<Ret(Args...)>::Invoke(Args... args) 
{
    PriorityQueue<Delegate<Ret(Args...)>> queue;

    while (!mDelegates.Empty())
    {
        mDelegates.Peek()(std::forward<Args>(args)...);
        
        queue.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(mDelegates.Peek())));
        mDelegates.Remove();
    }

    while (!queue.Empty())
    {
        mDelegates.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(queue.Peek())));
        queue.Remove();
    }
}

template <typename Ret, typename... Args>
template <typename F>
void Signal<Ret(Args...)>::operator()(const F &f, Args... args)
{
    Invoke(f, std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
template <typename F>
void Signal<Ret(Args...)>::Invoke(const F &f, Args... args)
{
    PriorityQueue<Delegate<Ret(Args...)>> queue;

    while (!mDelegates.Empty())
    {
        if (f(mDelegates.Peek()(std::forward<Args>(args)...)))
        {
            queue.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(mDelegates.Peek())));
            mDelegates.Remove();

            break;
        }
        else
        {
            queue.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(mDelegates.Peek())));
            mDelegates.Remove();
        }
    }

    while (!queue.Empty())
    {
        mDelegates.Insert(std::move(const_cast<Delegate<Ret(Args...)>&>(queue.Peek())));
        queue.Remove();
    }
}

#endif  // SIGNAL_H