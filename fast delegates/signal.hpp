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

/**** signal partial class template for function types ****/
template <typename Ret, typename... Args>
class Signal<Ret(Args...)>
{
public:
    template <Ret(*FreeFunction)(Args...)>
    void Bind();

    template <typename Type, Ret(Type::*PtrToMemFun)(Args...)>
    void Bind(Type &instance);
    
    template <typename Type, Ret(Type::*PtrToConstMemFun)(Args...) const>
    void Bind(Type &instance);

    template <typename Type>
    void Bind(Type &&funObj);

    explicit operator bool() const { return !mDelegates.empty(); }

    void operator()(Args... args) { for (auto &delegate : mDelegates) delegate(std::forward<Args>(args)...); }
    void Invoke(Args... args) { for (auto &delegate : mDelegates) delegate->Invoke(std::forward<Args>(args)...); }
private:
    std::vector<Delegate<Ret(Args...)>> mDelegates;
};

template <typename Ret, typename... Args>
template <Ret(*FreeFunction)(Args...)>
void Signal<Ret(Args...)>::Bind()
{
    Delegate<Ret(Args...)> delegate;
    delegate.template Bind<FreeFunction>();
    mDelegates.push_back(delegate);
}

template <typename Ret, typename... Args>
template <typename Type, Ret(Type::*PtrToMemFun)(Args...)>
void Signal<Ret(Args...)>::Bind(Type &instance)
{
    Delegate<Ret(Args...)> delegate;
    delegate.template Bind<Type, PtrToMemFun>(instance);
    mDelegates.push_back(delegate);
}
    
template <typename Ret, typename... Args>
template <typename Type, Ret(Type::*PtrToConstMemFun)(Args...) const>
void Signal<Ret(Args...)>::Bind(Type &instance)
{
    Delegate<Ret(Args...)> delegate;
    delegate.template Bind<Type, PtrToConstMemFun>(instance);
    mDelegates.push_back(delegate);
}
    
template <typename Ret, typename... Args>
template <typename Type>
void Signal<Ret(Args...)>::Bind(Type &&funObj)
{
    Delegate<Ret(Args...)> delegate;
    delegate.template Bind(std::forward<Type>(funObj));
    mDelegates.push_back(delegate);
}

#endif  // SIGNAL_H