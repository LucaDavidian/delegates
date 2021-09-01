#ifndef DELEGATE_H
#define DELEGATE_H

#include <utility>  
#include <type_traits>
#include <exception>
#include "callable.hpp"
#include "connection.hpp"

/***** delegate exceptions *****/
class DelegateNotBoundException : public std::exception
{
public:
    const char *what() const noexcept override
    {
        return "delegate not bound";
    }
};

class DelegateAlreadyBoundException : public std::exception
{
public:
    const char *what() const noexcept override
    {
        return "delegate already bound";
    }
};

class WrongCallableException : public std::exception
{
public:
    const char *what() const noexcept override
    {
        return "trying to disconnect wrong callable";
    }
};

/***** forward declaration (for friend declaration inside Delegate) *****/
template <typename Signature>
class Signal;

/**************** delegate ****************/

/**** delegate primary class template (not defined) ****/
template <typename Signature>
class Delegate;

/**** namespace scope swap ****/
template <typename Ret, typename... Args>
void swap(Delegate<Ret(Args...)> &d1, Delegate<Ret(Args...)> &d2)
{
    d1.Swap(d2);
}

/**** delegate partial class template specialization for function types ****/
template <typename Ret, typename... Args>
class Delegate<Ret(Args...)> 
{
friend class Signal<Ret(Args...)>;
public:
    Delegate() : mCallableWrapper(nullptr) {}

    Delegate(const Delegate &other) = delete;

    Delegate(Delegate &&other);

    ~Delegate();

    Delegate &operator=(Delegate const &other) = delete;

    Delegate &operator=(Delegate &&other);

    // template <typename T>
    // void Bind(T &instance, Ret (T::*ptrToMemFun)(Args...));

    // template <typename T>
    // void Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const);

    // note: by using a template type parameter as a pointer to member function
    // 1. there's no need for two separate functions (one for const member functions and
    // one for non-const member functions) and 
    // 2. the Bind function can accept member functions whose signature doesn't match 
    // exactly that of the Delegate (the arguments and return types must be convertible 
    // to those in the Delegate's signature)
    template <typename T, typename PtrToMemFun>
    void Bind(T &instance, PtrToMemFun ptrToMemFun);

    template <typename T>
    void Bind(T &&funObj);

    void Swap(Delegate &other);

    explicit operator bool() const { return mCallableWrapper != nullptr; }

    Ret operator()(Args... args);  

    Ret Invoke(Args... args);
private:
    CallableWrapper<Ret(Args...)> *mCallableWrapper; 
};

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::Delegate(Delegate &&other) : mCallableWrapper(other.mCallableWrapper)
{
    other.mCallableWrapper = nullptr;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::~Delegate() 
{
    delete mCallableWrapper; 
    mCallableWrapper = nullptr;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)> &Delegate<Ret(Args...)>::operator=(Delegate &&other)
{
    Delegate temp(std::move(other));
    Swap(temp);

    return *this;
}

// template <typename Ret, typename... Args>
// template <typename T>
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...))
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new MemFunCallableWrapper<T,Ret(Args...)>(instance, ptrToMemFun);
// }

// template <typename Ret, typename... Args>
// template <typename T>
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const)
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new ConstMemFunCallableWrapper<T,Ret(Args...)>(instance, ptrToConstMemFun);
// }

template <typename Ret, typename... Args>
template <typename T, typename PtrToMemFun>
void Delegate<Ret(Args...)>::Bind(T &instance, PtrToMemFun ptrToMemFun)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new MemFunCallableWrapper<Ret(Args...), T, PtrToMemFun>(instance, ptrToMemFun);
}

template <typename Ret, typename... Args>
template <typename T>
void Delegate<Ret(Args...)>::Bind(T &&funObj)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new FunObjCallableWrapper<Ret(Args...), std::remove_reference_t<T>>(std::forward<T>(funObj));  
}

template <typename Ret, typename... Args>
void Delegate<Ret(Args...)>::Swap(Delegate &other)
{
    CallableWrapper<Ret(Args...)> *temp = mCallableWrapper;
    mCallableWrapper = other.mCallableWrapper;
    other.mCallableWrapper = temp;
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::operator()(Args... args)
{
    if (!mCallableWrapper)
        throw DelegateNotBoundException();

    return mCallableWrapper->Invoke(std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::Invoke(Args... args)
{
    if (!mCallableWrapper)
        throw DelegateNotBoundException();

    return mCallableWrapper->Invoke(std::forward<Args>(args)...);
}

#endif  // DELEGATE_H