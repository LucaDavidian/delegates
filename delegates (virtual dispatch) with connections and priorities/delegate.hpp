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

template <typename Ret, typename... Args>
bool operator<(Delegate<Ret(Args...)> const &d1, Delegate<Ret(Args...)> const &d2)
{
    return d1.mPriority < d2.mPriority;
}

/**** delegate partial class template specialization for function types ****/
template <typename Ret, typename... Args>
class Delegate<Ret(Args...)> 
{
friend class Signal<Ret(Args...)>;
friend bool operator< <>(Delegate const &, Delegate const &);
public:
    Delegate() : mCallableWrapper(nullptr) {}

    Delegate(const Delegate &other) = delete;

    Delegate(Delegate &&other);

    ~Delegate();

    Delegate &operator=(Delegate const &other) = delete;

    Delegate &operator=(Delegate &&other);

    // template <typename T>
    // void Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority = -1);

    // template <typename T>
    // void Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority = -1);

    template <typename T, typename PtrToMemFun>
    std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>> Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority = -1);

    template <typename T>
    void Bind(T &&funObj, unsigned int priority = -1);

    void Swap(Delegate &other);

    explicit operator bool() const { return mCallableWrapper != nullptr; }

    Ret operator()(Args... args) const;  

    Ret Invoke(Args... args) const;
private:
    CallableWrapper<Ret(Args...)> *mCallableWrapper; 
    unsigned int mPriority;
};

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::Delegate(Delegate &&other) : mCallableWrapper(other.mCallableWrapper), mPriority(other.mPriority)
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
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority)
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new MemFunCallableWrapper<T,Ret(Args...)>(instance, ptrToMemFun);
//     mPriority = priority;
// }

// template <typename Ret, typename... Args>
// template <typename T>
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority)
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new ConstMemFunCallableWrapper<T,Ret(Args...)>(instance, ptrToConstMemFun);
//     mPriority = priority;
// }

template <typename Ret, typename... Args>
template <typename T, typename PtrToMemFun>
std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>> Delegate<Ret(Args...)>::Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new MemFunCallableWrapper<Ret(Args...), T, PtrToMemFun>(instance, ptrToMemFun);
    mPriority = priority;
}

template <typename Ret, typename... Args>
template <typename T>
void Delegate<Ret(Args...)>::Bind(T &&funObj, unsigned int priority)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new FunObjCallableWrapper<Ret(Args...), std::remove_reference_t<T>>(std::forward<T>(funObj));  
    mPriority = priority;
}

template <typename Ret, typename... Args>
void Delegate<Ret(Args...)>::Swap(Delegate &other)
{
    CallableWrapper<Ret(Args...)> *callableTemp = mCallableWrapper;
    mCallableWrapper = other.mCallableWrapper;
    other.mCallableWrapper = callableTemp;

    unsigned int priorityTemp = mPriority;
    mPriority = other.mPriority;
    other.mPriority = priorityTemp;
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::operator()(Args... args) const
{
    if (!mCallableWrapper)
        throw DelegateNotBoundException();

    return mCallableWrapper->Invoke(std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::Invoke(Args... args) const
{
    if (!mCallableWrapper)
        throw DelegateNotBoundException();

    return mCallableWrapper->Invoke(std::forward<Args>(args)...);
}

#endif  // DELEGATE_H