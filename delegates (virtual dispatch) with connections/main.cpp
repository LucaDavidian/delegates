#include "signal.hpp"
#include <iostream>
#include <vector>

SIGNAL_RET_ONE_PARAM(MySig, int, double);
MySig sig;

int FreeFunction(int, int) 
{
    std::cout << "in free function" << std::endl;

    return 1;
}

class MyClass
{
public:
    MyClass(int i) : i(i) 
    {
        mConnections.push_back(sig.Bind(*this, &MyClass::MemberFunction));
        mConnections.push_back(sig.Bind(*this, &MyClass::ConstMemberFunction));
        mConnections.push_back(sig.Bind(&MyClass::StaticMemberFunction));
        mConnections.push_back(sig.Bind(*this));
        mConnections.push_back(sig.Bind(*static_cast<MyClass const*>(this)));
    }

    ~MyClass() { for (auto &connection : mConnections) connection.Disconnect(); }

    int MemberFunction(double d) { std::cout << "in member function" << std::endl; return int(++i * d); }
    int ConstMemberFunction(double d) const { std::cout << "in const member function" << std::endl; return int(i * d); }
    int operator()(double d) { std::cout << "in overloaded function call operator" << std::endl; return (int)(++i + d); }
    int operator()(double d) const { std::cout << "in const overloaded function call operator" << std::endl; return (int)(i + d); }
    static int StaticMemberFunction(double d) { std::cout << "in static member function" << std::endl; return int(10 + d); }
private:
    int i;
    std::vector<Connection> mConnections;
};

int main(int argc, char *argv[])
{
    {
        MyClass mc(10);

        sig(1.2);
    }

    std::cout << "**********************" << std::endl;

    sig(1.2);  // empty signal

    std::cout << "**********************" << std::endl;

    int i = 10;
    sig.Bind([i](double d) mutable -> int { std::cout << "in temp lambda" << std::endl; return ++i; });

    auto lambda = [&i](double) { std::cout << "in lambda" << std::endl; return ++i; };
    sig.Bind(lambda);
    
    sig(1.20);


    return 0;
}