#ifndef CALLABLE_WRAPPER_H
#define CALLABLE_WRAPPER_H

#include "../../tuple/tuple.hpp"

/***** base callable wrapper class *****/
template <typename Signature>
class CallableWrapper;

template <typename Ret, typename... Args>
class CallableWrapper<Ret(Args...)>
{
public:
    virtual ~CallableWrapper() = default;

    virtual Ret Invoke(Args... args) = 0;
protected:
    CallableWrapper() = default;
};

/***** wrapper around a non-const member function *****/
template <typename T, typename Signature, typename... Payload>
class MemFunCallableWrapper;

template <typename T, typename Ret, typename... Args, typename... Payload>
class MemFunCallableWrapper<T, Ret(Args...), Payload...> : public CallableWrapper<Ret(Args...)>
{
private:
    using PtrToMemFun = Ret (T::*)(Args...);
public:
    MemFunCallableWrapper(T &instance, PtrToMemFun ptrToMemFun, Payload... payload) : mInstance(instance), mPtrToMemFun(ptrToMemFun), mPayload(payload...) {}

    Ret Invoke(Args... args) override 
    {  
        mArguments = Tuple<Args...>(args...); 
        return InvokeImpl(mPayloadSequence, mArgsSequence); 
    }
private:
    T &mInstance;
    PtrToMemFun mPtrToMemFun;

    Tuple<Payload...> mPayload;
    MakeIndexSequence<sizeof...(Payload)> mPayloadSequence;
    Tuple<Args...> mArguments;
    MakeIndexSequenceFrom<sizeof...(Payload), sizeof...(Args)> mArgsSequence;

    template <std::size_t... PayloadSequence, std::size_t... ArgsSequence>
    Ret InvokeImpl(IndexSequence<PayloadSequence...>, IndexSequence<ArgsSequence...>)
    {
        return (mInstance.*mPtrToMemFun)(Get<PayloadSequence>(mPayload)..., Get<ArgsSequence>(mArguments)...);
    }
};

/***** wrapper around a const member function *****/
template <typename T, typename Signature, typename... Payload>
class ConstMemFunCallableWrapper;

template <typename T, typename Ret, typename... Args, typename... Payload>
class ConstMemFunCallableWrapper<T, Ret(Args...), Payload...> : public CallableWrapper<Ret(Args...)>
{
private:
    using PtrToConstMemFun = Ret (T::*)(Args...) const;
public:
    ConstMemFunCallableWrapper(T &instance, PtrToConstMemFun ptrToConstMemFun, Payload... payload) : mInstance(instance), mPtrToConstMemFun(ptrToConstMemFun), mPayload(payload...) {}
    
    Ret Invoke(Args... args) override 
    {  
        mArguments = Tuple<Args...>(args...); 
        return InvokeImpl(mPayloadSequence, mArgsSequence); 
    }
private:
    T &mInstance;
    PtrToConstMemFun mPtrToConstMemFun;

    Tuple<Payload...> mPayload;
    MakeIndexSequence<sizeof...(Payload)> mPayloadSequence;
    Tuple<Args...> mArguments;
    MakeIndexSequenceFrom<sizeof...(Payload), sizeof...(Args)> mArgsSequence;

    template <std::size_t... PayloadSequence, std::size_t... ArgsSequence>
    Ret InvokeImpl(IndexSequence<PayloadSequence...>, IndexSequence<ArgsSequence...>)
    {
        return (mInstance.*mPtrToConstMemFun)(Get<PayloadSequence>(mPayload)..., Get<ArgsSequence>(mArguments)...);
    }
};

/***** wrapper around a function object/lambda *****/
template <typename T, typename Signature, typename... Payload>
class FunObjCallableWrapper;

template <typename T, typename Ret, typename... Args, typename... Payload>
class FunObjCallableWrapper<T,Ret(Args...), Payload...> : public CallableWrapper<Ret(Args...)>
{
public:
    FunObjCallableWrapper(T &funObject, Payload... payload) : mFunObject(&funObject), mPayload(payload...), mAllocated(false) {}
    FunObjCallableWrapper(T &&funObject, Payload... payload) : mFunObject(new T(std::move(funObject))), mPayload(payload...), mAllocated(true) {} 

    ~FunObjCallableWrapper() { Destroy(); }

    Ret Invoke(Args... args) override 
    {  
        mArguments = Tuple<Args...>(args...); 
        return InvokeImpl(mPayloadSequence, mArgsSequence); 
    }
private:
    template <typename U = T, typename = std::enable_if_t<std::is_function<U>::value>>                  // dummy type param defaulted to T (SFINAE)
    void Destroy() {}
    template <typename U = T, typename = std::enable_if_t<!std::is_function<U>::value>, typename = T>   // dummy type param defaulted to T (SFINAE) + extra type param (for overloading)
    void Destroy() { if (mAllocated) delete mFunObject; }                                               // SFINAE-out if T has function type

    T *mFunObject;
    bool mAllocated;

    Tuple<Payload...> mPayload;
    MakeIndexSequence<sizeof...(Payload)> mPayloadSequence;
    Tuple<Args...> mArguments;
    MakeIndexSequenceFrom<sizeof...(Payload), sizeof...(Args)> mArgsSequence;

    template <std::size_t... PayloadSequence, std::size_t... ArgsSequence>
    Ret InvokeImpl(IndexSequence<PayloadSequence...>, IndexSequence<ArgsSequence...>)
    {
        return (*mFunObject)(Get<PayloadSequence>(mPayload)..., Get<ArgsSequence>(mArguments)...);
    }
};

#endif  // CALLABLE_WRAPPER_H