#ifndef CALLABLE_WRAPPER_H
#define CALLABLE_WRAPPER_H

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
// template <typename T, typename Signature>
// class MemFunCallableWrapper;

// template <typename T,typename Ret, typename... Args>
// class MemFunCallableWrapper<T, Ret(Args...)> : public CallableWrapper<Ret(Args...)>
// {
// private:
//     using PtrToMemFun = Ret (T::*)(Args...);
// public:
//     MemFunCallableWrapper(T &instance, PtrToMemFun ptrToMemFun) : mInstance(instance), mPtrToMemFun(ptrToMemFun) {}

//     Ret Invoke(Args... args) override {  return (mInstance.*mPtrToMemFun)(std::forward<Args>(args)...); }
// private:
//     T &mInstance;
//     PtrToMemFun mPtrToMemFun;
// };

/***** wrapper around a const member function *****/
// template <typename T, typename Signature>
// class ConstMemFunCallableWrapper;

// template <typename T,typename Ret, typename... Args>
// class ConstMemFunCallableWrapper<T, Ret(Args...)> : public CallableWrapper<Ret(Args...)>
// {
// private:
//     using PtrToConstMemFun = Ret (T::*)(Args...) const;
// public:
//     ConstMemFunCallableWrapper(T &instance, PtrToConstMemFun ptrToConstMemFun) : mInstance(instance), mPtrToConstMemFun(ptrToConstMemFun) {}
    
//     Ret Invoke(Args... args) override {  return (mInstance.*mPtrToConstMemFun)(std::forward<Args>(args)...); }
// private:
//     T &mInstance;
//     PtrToConstMemFun mPtrToConstMemFun;
// };

/***** wrapper around a (possibly const) member function (this implementation provides automatic arguments and return type conversions) *****/
template <typename Signature, typename T, typename PtrToMemFun>
class MemFunCallableWrapper;

template <typename Ret, typename... Args, typename T, typename PtrToMemFun>
class MemFunCallableWrapper<Ret(Args...), T, PtrToMemFun> : public CallableWrapper<Ret(Args...)>
{
public:
    MemFunCallableWrapper(T &instance, PtrToMemFun ptrToMemFun) : mInstance(instance), mPtrToMemFun(ptrToMemFun) {}

    Ret Invoke(Args... args) override { return (mInstance.*mPtrToMemFun)(std::forward<Args>(args)...); }
private:
    T &mInstance;
    PtrToMemFun mPtrToMemFun;
};

/***** wrapper around a function object/lambda *****/
template <typename Signature, typename T>
class FunObjCallableWrapper;

template <typename Ret, typename... Args, typename T>
class FunObjCallableWrapper<Ret(Args...), T> : public CallableWrapper<Ret(Args...)>
{
public:
    FunObjCallableWrapper(T &funObject) : mFunObject(&funObject), mAllocated(false) {}
    FunObjCallableWrapper(T &&funObject) : mFunObject(new T(std::move(funObject))), mAllocated(true) {} 

    ~FunObjCallableWrapper() { Destroy(); }

    Ret Invoke(Args... args) override { return (*mFunObject)(std::forward<Args>(args)...); }
private:
    template <typename U = T, typename = std::enable_if_t<std::is_function<U>::value>>                  // dummy type param defaulted to T (SFINAE)
    void Destroy() {}
    template <typename U = T, typename = std::enable_if_t<!std::is_function<U>::value>, typename = T>   // dummy type param defaulted to T (SFINAE) + extra type param (for overloading)
    void Destroy() { if (mAllocated) delete mFunObject; }                                               // SFINAE-out if T has function type

    T *mFunObject;
    bool mAllocated;
};

#endif  // CALLABLE_WRAPPER_H