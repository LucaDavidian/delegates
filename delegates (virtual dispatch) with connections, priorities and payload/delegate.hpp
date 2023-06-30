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
template <typename Signature>
void swap(Delegate<Signature> &d1, Delegate<Signature> &d2)
{
    d1.Swap(d2);
}

template <typename Signature>
bool operator<(Delegate<Signature> const &d1, Delegate<Signature> const &d2)
{
    return d1.mPriority < d2.mPriority;
}

/**** delegate partial class template specialization for function types ****/
template <typename Ret, typename... Args>
class Delegate<Ret(Args...)> 
{
friend class Signal<Ret(Args...)>;
friend bool operator< <Ret(Args...)>(Delegate const &, Delegate const &);
public:
    Delegate() : mCallableWrapper(nullptr) {}

    Delegate(const Delegate &other) = delete;

    Delegate(Delegate &&other);

    ~Delegate();

    Delegate &operator=(Delegate const &other) = delete;

    Delegate &operator=(Delegate &&other);

    // template <typename T, typename... Payload>
    // void Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority, Payload&&... payload);

    // template <typename T, typename... Payload>
    // void Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority, Payload&&... payload);

    // note: by using a template type parameter as a pointer to member function
    // 1. there's no need for two separate Bind member functions (one for const member functions 
    //    and one for non-const member functions) and 
    // 2. the Bind function can accept member functions whose signature doesn't match 
    //    exactly that of the delegate (the delegate musr accept parameters that can be converted to
    //    those in the bound function signature and the bound function return type must be convertible 
    //    to that in the delegate's signature)
    template <typename T, typename PtrToMemFun, typename... Payload>
    std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>> Bind(T &instance, PtrToMemFun ptrToConstMemFun, unsigned int priority, Payload&&... payload);

    template <typename T, typename... Payload>
    void Bind(T &&funObj, unsigned int priority, Payload&&... payload);

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
// template <typename T, typename... Payload>
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToMemFun)(Args...), unsigned int priority, Payload&&... payload)
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new MemFunCallableWrapper<T,Ret(Args...), Payload...>(instance, ptrToMemFun, std::forward<Payload>(payload)...);
//     mPriority = priority;
// }

// template <typename Ret, typename... Args>
// template <typename T, typename... Payload>
// void Delegate<Ret(Args...)>::Bind(T &instance, Ret (T::*ptrToConstMemFun)(Args...) const, unsigned int priority, Payload&&... payload)
// {
//     if (mCallableWrapper)
//         throw DelegateAlreadyBoundException();

//     mCallableWrapper = new ConstMemFunCallableWrapper<T,Ret(Args...), Payload...>(instance, ptrToConstMemFun, std::forward<Payload>(payload)...);
//     mPriority = priority;
// }

template <typename Ret, typename... Args>
template <typename T, typename PtrToMemFun, typename... Payload>
std::enable_if_t<std::is_member_function_pointer_v<PtrToMemFun>> Delegate<Ret(Args...)>::Bind(T &instance, PtrToMemFun ptrToMemFun, unsigned int priority, Payload&&... payload)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new MemFunCallableWrapper<Ret(Args...), T, PtrToMemFun, Payload...>(instance, ptrToMemFun, std::forward<Payload>(payload)...);
    mPriority = priority;
}

template <typename Ret, typename... Args>
template <typename T, typename... Payload>
void Delegate<Ret(Args...)>::Bind(T &&funObj, unsigned int priority, Payload&&... payload)
{
    if (mCallableWrapper)
        throw DelegateAlreadyBoundException();

    mCallableWrapper = new FunObjCallableWrapper<Ret(Args...), std::remove_reference_t<T>, Payload...>(std::forward<T>(funObj), std::forward<Payload>(payload)...);  
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