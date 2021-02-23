#ifndef DELEGATE_H
#define DELEGATE_H

#include <type_traits>
#include <utility>
#include <new>
#include <cstddef>

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

    template <Ret(*FreeFunction)(Args...)>
    void Bind();

    template <typename Type, Ret(Type::*PtrToMemFun)(Args...)>
    void Bind(Type &instance);
    
    template <typename Type, Ret(Type::*PtrToConstMemFun)(Args...) const>
    void Bind(Type &instance);
    
    template <typename Type>
    void Bind(Type &funObj);

    template <typename Type>
    void Bind(Type &&funObj);

    void Swap(Delegate &other);

    explicit operator bool() const { return mFunction != nullptr; }

    Ret operator()(Args... args) { return mFunction(&mData, std::forward<Args>(args)...); }
    Ret Invoke(Args... args) { return mFunction(&mData, std::forward<Args>(args)...); }
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

    DestroyStorageFunction mDestroyStorage = nullptr;
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

    /**** stub functions ****/
    template <Ret(*FreeFunction)(Args...)>
    static Ret Stub(void *data, Args... args)
    {
        return FreeFunction(std::forward<Args>(args)...);
    }

    template <typename Type, Ret(Type::*PtrToMemFun)(Args...)>
    static Ret Stub(void *data, Args... args)
    {
        Storage *storage = static_cast<Storage*>(data);
        Type *instance = *reinterpret_cast<Type**>(storage);

        return (instance->*PtrToMemFun)(std::forward<Args>(args)...);
    }

    template <typename Type, Ret(Type::*PtrToConstMemFun)(Args...) const>
    static Ret Stub(void *data, Args... args)
    {
        Storage *storage = static_cast<Storage*>(data);
        Type *instance = *reinterpret_cast<Type**>(storage);

        return (instance->*PtrToConstMemFun)(std::forward<Args>(args)...);
    }

    template <typename Type>
    static Ret Stub(void *data, Args... args)
    {
        Storage *storage = static_cast<Storage*>(data);
        Type *instance = *reinterpret_cast<Type**>(storage);

        return (*instance)(std::forward<Args>(args)...);    
    }

    template <typename Type, typename>
    static Ret Stub(void *data, Args... args)
    {
        Storage *storage = static_cast<Storage*>(data);
        Type *instance = reinterpret_cast<Type*>(storage);

        return (*instance)(std::forward<Args>(args)...);   
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
template <Ret(*FreeFunction)(Args...)>
void Delegate<Ret(Args...)>::Bind()
{
    new(&mData) std::nullptr_t(nullptr);

    mFunction = &Stub<FreeFunction>;
}

template <typename Ret, typename... Args>
template <typename Type, Ret(Type::*PtrToMemFun)(Args...)>
void Delegate<Ret(Args...)>::Bind(Type &instance)
{
    new(&mData) Type*(&instance);

    mFunction = &Stub<Type, PtrToMemFun>;
}
    
template <typename Ret, typename... Args>
template <typename Type, Ret(Type::*PtrToConstMemFun)(Args...) const>
void Delegate<Ret(Args...)>::Bind(Type &instance)
{
    new(&mData) Type*(&instance);

    mFunction = &Stub<Type, PtrToConstMemFun>;
}

template <typename Ret, typename... Args>
template <typename Type>
void Delegate<Ret(Args...)>::Bind(Type &funObj)   
{
    new(&mData) Type*(&funObj);    

    mFunction = &Stub<Type>;
}
    
template <typename Ret, typename... Args>
template <typename Type>
void Delegate<Ret(Args...)>::Bind(Type &&funObj)
{
    static_assert(sizeof(Type) <= sizeof(void*));

    new(&mData) Type(std::move(funObj));    

    mFunction = &Stub<Type, Type>;

    mDestroyStorage = &DestroyStorage<Type>;
    mCopyStorage = &CopyStorage<Type>;
    mMoveStorage = &MoveStorage<Type>;

    mStored = true;
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

#endif  // DELEGATE_H