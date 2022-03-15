#ifndef SIGNAL_H
#define SIGNAL_H

#include "delegate.hpp"
#define TYPE_PARAM_HEAP_QUEUE
#include "../../data structures/ADT/priority queue/priority_queue.hpp"
#include <type_traits>

/***** signal typedefs *****/
#define SIGNAL(SignalType)                                  typedef Signal<void()> SignalType
#define SIGNAL_ONE_PARAM(SignalType, par0)                  typedef Signal<void(par0)> SignalType
#define SIGNAL_TWO_PARAM(SignalType, par0, par1)            typedef Signal<void(par0, par1)> SignalType

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
    // template <typename T>
    // Connection Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority = -1);

    // template <typename T>
    // Connection Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority = -1);

    // note: by using a template type parameter as a pointer to member function
    // 1. there's no need for two separate Bind member functions (one for const member functions 
    //    and one for non-const member functions) and 
    // 2. the Bind function can accept member functions whose signature doesn't match 
    //    exactly that of the delegate (the delegate musr accept parameters that can be converted to
    //    those in the bound function signature and the bound function return type must be convertible 
    //    to that in the delegate's signature)
    template <typename T, typename PtrToMemFun>
    std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>, Connection> Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority = -1);

    template <typename T>
    Connection Bind(T &&funObj, unsigned int priority = -1);

    explicit operator bool() const { return !mDelegates.empty(); }

    void operator()(Args... args); 
    
    void Invoke(Args... args);

    // call all delegates until f doesn't return true
    template <typename F>
    void operator()(F const &f, Args... args);

    // call all delegates until f doesn't return true
    template <typename F>
    void Invoke(const F &f, Args... args);
private:
    void Unbind(CallableWrapper<Ret(Args...)> *callableWrapper);

    PriorityQueue<Delegate<Ret(Args...)>> mDelegates;
};

// template <typename Ret, typename... Args>
// template <typename T>
// Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority)
// {   
//     Delegate<Ret(Args...)> delegate;
//     delegate.Bind(instance, ptrToMemFun, priority);
//     CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
//     mDelegates.Insert(std::move(delegate));

//     return Connection(this, callable); 
// }

// template <typename Ret, typename... Args>
// template <typename T>
// Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority)
// {
//     Delegate<Ret(Args...)> delegate;
//     delegate.Bind(instance, ptrToConstMemFun, priority);
//     CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
//     mDelegates.Insert(std::move(delegate));

//     return Connection(this, callable); 
// }

template <typename Ret, typename... Args>
template <typename T, typename PtrToMemFun>
std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>, Connection> Signal<Ret(Args...)>::Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority)
{
    Delegate<Ret(Args...)> delegate;
    delegate.Bind(instance, ptrToMemFun, priority);
    CallableWrapper<Ret(Args...)> *callable = delegate.mCallableWrapper;
    mDelegates.Insert(std::move(delegate));

    return Connection(this, callable); 
}
    
template <typename Ret, typename... Args>
template <typename T>
Connection Signal<Ret(Args...)>::Bind(T &&funObj, unsigned int priority)
{
    Delegate<Ret(Args...)> delegate;
    delegate.Bind(std::forward<T>(funObj), priority);
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