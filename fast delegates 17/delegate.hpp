#ifndef DELEGATE_H
#define DELEGATE_H

#include <utility>
#include <type_traits>
#include <functional>

/***** delegate typedefs *****/
#define DELEGATE(delegateName)                         typedef Delegate<void()> delegateName
#define DELEGATE_ONE_PARAM(delegateName, par0)         typedef Delegate<void(par0)> delegateName
#define DELEGATE_TWO_PARAM(delegateName, par0, par1)   typedef Delegate<void(par0, par1)> delegateName

#define DELEGATE_RET(delegateName, ret)                         typedef Delegate<ret()> delegateName
#define DELEGATE_RET_ONE_PARAM(delegateName, ret, par0)         typedef Delegate<ret(par0)> delegateName
#define DELEGATE_RET_TWO_PARAM(delegateName, ret, par0, par1)   typedef Delegate<ret(par0, par1)> delegateName

/***** multicast delegate typedefs *****/
#define MULTICAST_DELEGATE(delegateName)                         typedef MulticastDelegate<void()> delegateName
#define MULTICAST_DELEGATE_ONE_PARAM(delegateName, par0)         typedef MulticastDelegate<void(par0)> delegateName
#define MULTICAST_DELEGATE_TWO_PARAM(delegateName, par0, par1)   typedef MulticastDelegate<void(par0, par1)> delegateName

#define MULTICAST_DELEGATE_RET(delegateName, ret)                         typedef MulticastDelegate<ret()> delegateName
#define MULTICAST_DELEGATE_RET_ONE_PARAM(delegateName, ret, par0)         typedef MulticastDelegate<ret(par0)> delegateName
#define MULTICAST_DELEGATE_RET_TWO_PARAM(delegateName, ret, par0, par1)   typedef MulticastDelegate<ret(par0, par1)> delegateName

/**** delegate primary class template (not defined) ****/
template <typename Signature>
class Delegate;

/**** namespace scope swap function ****/
template <typename Signature>
void swap(Delegate<Signature> &d1, Delegate<Signature> &d2)
{   
    d1.Swap(d2);
}

/**** delegate partial class template for function types ****/
template <typename Ret, typename... Args>
class Delegate<Ret(Args...)>
{
public:
    Delegate();

    Delegate(Delegate const &other);

    Delegate(Delegate &&other);

    ~Delegate();

    Delegate &operator=(Delegate const &other);

    Delegate &operator=(Delegate &&other);

    template <auto FreeFunction>
    void Bind();

    template <auto Callable, typename Type>
    void Bind(Type &instance);

    template <typename Type>
    void Bind(Type &&funObj);

    void Swap(Delegate &other);

    explicit operator bool() const { return mFunction; } 

    Ret operator()(Args... args);
    Ret Invoke(Args... args);
private:
    //typedef typename std::aligned_storage<sizeof(void*), alignof(void*)>::type Storage;
    using Storage = std::aligned_storage_t<sizeof(void*), alignof(void*)> ;
    using Function = Ret(*)(void*, Args...);
    
    Storage mData;
    Function mFunction;

    bool mStored = false;

    using DestroyStorageFunction = void(*)(Delegate*);
    using CopyStorageFunction = void(*)(const Delegate*, Delegate*);
    using MoveStorageFunction = void(*)(Delegate*, Delegate*);

    DestroyStorageFunction  mDestroyStorage = nullptr;
    CopyStorageFunction mCopyStorage = nullptr;
    MoveStorageFunction mMoveStorage = nullptr;

    /**** helper function templates for special member functions ****/
    template <typename Type>
    static void DestroyStorage(Delegate *delegate)
    {
        reinterpret_cast<Type*>(&delegate->mData)->~Type();
    }

    template <typename Type>
    static void CopyStorage(const Delegate *src, Delegate *dst)
    {
        new(&dst->mData) Type(*reinterpret_cast<const Type*>(&src->mData));
    }

    template <typename Type>
    static void MoveStorage(Delegate *src, Delegate *dst)
    {
        new(&dst->mData) Type(std::move(*reinterpret_cast<Type*>(&src->mData)));
    }
};

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::Delegate()
{
    new(&mData) std::nullptr_t(nullptr);

    mFunction = nullptr;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::Delegate(Delegate const &other)
{
    if (other.mStored)
    {
        other.mCopyStorage(&other, this);

        mDestroyStorage = other.mDestroyStorage;
        mCopyStorage = other.mCopyStorage;
        mMoveStorage = other.mMoveStorage;

        mStored = true;
    }
    else
        mData = other.mData;

    mFunction = other.mFunction;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::Delegate(Delegate &&other)
{
    if (other.mStored)
    {
        other.mMoveStorage(&other, this);

        mDestroyStorage = other.mDestroyStorage;
        mCopyStorage = other.mCopyStorage;
        mMoveStorage = other.mMoveStorage;

        mStored = true;
    }
    else
        mData = other.mData;

    mFunction = other.mFunction;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)>::~Delegate()
{
    if (mStored)
        mDestroyStorage(this);
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)> &Delegate<Ret(Args...)>::operator=(Delegate const &other)
{
    Delegate temp(other);
    Swap(temp);

    return *this;
}

template <typename Ret, typename... Args>
Delegate<Ret(Args...)> &Delegate<Ret(Args...)>::operator=(Delegate &&other)
{
    Delegate temp(std::move(other));
    Swap(temp);

    return *this;
}

template <typename Ret, typename... Args>
template <auto FreeFunction>
void Delegate<Ret(Args...)>::Bind()
{
    new(&mData) std::nullptr_t(nullptr);

    mFunction = +[](void*, Args... args) -> Ret
            {
                return FreeFunction(std::forward<Args>(args)...);
            };
}

template <typename Ret, typename... Args>
template <auto Callable, typename Type>
void Delegate<Ret(Args...)>::Bind(Type &instance)
{
    new(&mData) Type*(&instance);

    mFunction = +[](void *data, Args... args) -> Ret
            {
                Storage *storage = static_cast<Storage*>(data);
                Type *instance = *reinterpret_cast<Type**>(storage);
                return std::invoke(Callable, instance, std::forward<Args>(args)...);
            };
}

template <typename Ret, typename... Args>
template <typename Type>
void Delegate<Ret(Args...)>::Bind(Type &&funObj)
{  
    static_assert(sizeof(Type) <= sizeof(void*));
    
    if constexpr (std::is_lvalue_reference<Type>::value)
    {
        new(&mData) std::remove_reference_t<Type>*(&funObj);

        mFunction = +[](void *data, Args... args) -> Ret
            {
                Storage *storage = static_cast<Storage*>(data);
                std::remove_reference_t<Type> *instance = *reinterpret_cast<std::remove_reference_t<Type>**>(storage);
                return std::invoke(*instance, std::forward<Args>(args)...);
            };  
    }
    else
    {
        new(&mData) Type(std::move(funObj));
        mDestroyStorage = &DestroyStorage<Type>;
        mStored = true;

        mFunction = +[](void *data, Args... args) -> Ret
            {
                Storage *storage = static_cast<Storage*>(data);
                std::remove_reference_t<Type> *instance = reinterpret_cast<std::remove_reference_t<Type>*>(storage);
                return std::invoke(*instance, std::forward<Args>(args)...);
            };  
    } 
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::operator()(Args... args)
{
    return mFunction(&mData, std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret Delegate<Ret(Args...)>::Invoke(Args... args)
{
    return mFunction(&mData, std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
void Delegate<Ret(Args...)>::Swap(Delegate &other)
{
    if (other.mStored)
        other.mCopyStorage(&other, this);
    else
         mData = other.mData;

    Function tempFunction = mFunction;
    mFunction = other.mFunction;
    other.mFunction = tempFunction;

    bool tempStored = mStored;
    mStored = other.mStored;
    other.mStored = tempStored;

    DestroyStorageFunction tempDestroyFun = mDestroyStorage;
    mDestroyStorage = other.mDestroyStorage;
    other.mDestroyStorage = tempDestroyFun;

    CopyStorageFunction tempCopyFun = mCopyStorage;
    mCopyStorage = other.mCopyStorage;
    other.mCopyStorage = tempCopyFun;
    
    MoveStorageFunction tempMoveFun = mMoveStorage;
    mMoveStorage = other.mMoveStorage;
    other.mMoveStorage = tempMoveFun;
}

/**************** multicast delegate ****************/
#include <vector>

/**** multicast delegate primary class template (not defined) ****/
template <typename Signature>
class MulticastDelegate;

/**** delegate partial class template for function types ****/
template <typename Ret, typename... Args>
class MulticastDelegate<Ret(Args...)>
{
public:
    MulticastDelegate(std::size_t size = 10U)  { mDelegates.reserve(size); }
    
    template <auto Callable>
    void Bind();

    template <auto Callable, typename Type>
    void Bind(Type &instance);

    template <typename Type>
    void Bind(Type &&funObj);

    explicit operator bool() const { return !mDelegates.empty(); }

    void operator()(Args... args) { for (auto &delegate : mDelegates) delegate(std::forward<Args>(args)...); }
    void Invoke(Args... args) { for (auto &delegate : mDelegates) delegate.Invoke(std::forward<Args>(args)...); }
private:
    std::vector<Delegate<Ret(Args...)>> mDelegates;
};

template <typename Ret, typename... Args>
template <auto Callable>
void MulticastDelegate<Ret(Args...)>::Bind()
{
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(delegate);

    mDelegates.back().template Bind<Callable>();
}

template <typename Ret, typename... Args>
template <auto Callable, typename Type>
void MulticastDelegate<Ret(Args...)>::Bind(Type &instance)
{
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(delegate);

    mDelegates.back().template Bind<Callable>(instance);
}

template <typename Ret, typename... Args>
template <typename Type>
void MulticastDelegate<Ret(Args...)>::Bind(Type &&funObj)
{
    Delegate<Ret(Args...)> delegate;
    mDelegates.push_back(delegate);

    mDelegates.back().Bind(std::forward<Type>(funObj));
}

#endif  // DELEGATE_H