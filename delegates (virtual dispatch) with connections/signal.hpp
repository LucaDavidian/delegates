#ifndef SIGNAL_H
#define SIGNAL_H

#include "delegate.hpp"
#include <vector>

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
    template <typename T>
    Connection Bind(T &instance, Ret (T::*ptrToMemFun)(Args...));

    template <typename T>
    Connection Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const);

    template <typename T>
    Connection Bind(T &&funObj);

    explicit operator bool() const { return !mDelegates.empty(); }

    void operator()(Args... args) { for (auto &delegate : mDelegates) delegate(std::forward<Args>(args)...); }  
    
    void Invoke(Args... args) { for (auto &delegate : mDelegates) delegate.Invoke(std::forward<Args>(args)...); }
private:
    void Unbind(CallableWrapper<Ret(Args...)> *callableWrapper);

    std::vector<Delegate<Ret(Args...)>> mDelegates;
};

template <typename Ret, typename... Args>
template <typename T>
Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...))
{   
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(std::move(delegate));
    mDelegates.back().Bind(instance, ptrToMemFun);

    return Connection(this, mDelegates.back().mCallableWrapper);
}

template <typename Ret, typename... Args>
template <typename T>
Connection Signal<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const)
{
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(std::move(delegate));
    mDelegates.back().Bind(instance, ptrToConstMemFun);

    return Connection(this, mDelegates.back().mCallableWrapper); 
}
    
template <typename Ret, typename... Args>
template <typename T>
Connection Signal<Ret(Args...)>::Bind(T &&funObj)
{
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(std::move(delegate));
    mDelegates.back().Bind(std::forward<T>(funObj));

    return Connection(this, mDelegates.back().mCallableWrapper); 
}

template <typename Ret, typename... Args>
void Signal<Ret(Args...)>::Unbind(CallableWrapper<Ret(Args...)> *callableWrapper)
{
    for (auto it = mDelegates.begin(), end = mDelegates.end(); it != end; ++it)
        if (it->mCallableWrapper == callableWrapper)
        {
            mDelegates.erase(it);
            return;
        }
}

#endif  // SIGNAL_H