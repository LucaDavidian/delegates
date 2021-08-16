#ifndef CONNECTION_H
#define CONNECTION_H

template <typename Signature>
class Signal;

template <typename Signature>
class CallableWrapper;

class Connection
{
public:
    Connection() : mSignal(nullptr), mCallableWrapper(nullptr), mDisconnectFunction(nullptr) {}
    
    template <typename Ret, typename... Args>
    Connection(Signal<Ret(Args...)> *signal, CallableWrapper<Ret(Args...)> *callableWrapper) : mSignal(signal), mCallableWrapper(callableWrapper), mDisconnectFunction(&DisconnectFunction<Ret, Args...>) {}
    
    void Disconnect() 
    { 
        if (mDisconnectFunction)
            mDisconnectFunction(mSignal, mCallableWrapper); 
    }
private:
    void (*mDisconnectFunction)(void*, void*);
    void *mSignal;
    void *mCallableWrapper;
    
    template <typename Ret, typename... Args>
    static void DisconnectFunction(void *signal, void *callableWrapper)
    {
        static_cast<Signal<Ret(Args...)>*>(signal)->Unbind(static_cast<CallableWrapper<Ret(Args...)>*>(callableWrapper));
    }
};

class AutoConnection : public Connection
{
public:
    ~AutoConnection() 
    {
        Disconnect();        
    }
};

#endif  // CONNECTION_H

#endif  // CONNECTION_H