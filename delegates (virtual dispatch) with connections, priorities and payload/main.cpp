#include "signal.hpp"
#include <iostream>
#include <vector>

SIGNAL_RET_TWO_PARAM(MySig, int, double, int);
MySig sig;

int FreeFunction(double, int) 
{
    std::cout << "in free function" << std::endl;

    return 1;
}

class MyClass
{
public:
    MyClass()
    {
        mConnections.push_back(sig.Bind(*this, &MyClass::MemberFunction, -1));
        mConnections.push_back(sig.Bind(*this, &MyClass::ConstMemberFunction, -1));
        mConnections.push_back(sig.Bind(&MyClass::StaticMemberFunction, 3, 0.2));
        mConnections.push_back(sig.Bind(*this, 2));
        mConnections.push_back(sig.Bind(*static_cast<MyClass const*>(this), 1, 3.21, 12));
    }

    ~MyClass() { for (auto &connection : mConnections) connection.Disconnect(); }

    int MemberFunction(double d, int i) { std::cout << "in member : " << d << " " << i << std::endl; return int(++i * d); }
    int ConstMemberFunction(double d, int i) const { std::cout << "in const member function: " << d << " " << i << std::endl; return int(i * d); }
    int operator()(double d, int i) { std::cout << "in overloaded function call operator: " << d << " " << i << std::endl; return (int)(++i + d); }
    int operator()(double d, int i) const { std::cout << "in const overloaded function call operator: " << d << " " << i << std::endl; return (int)(i + d); }
    static int StaticMemberFunction(double d, int i) { std::cout << "in static member function: " << d << " " << i << std::endl; return int(10 + d); }
private:
    int i;
    std::vector<Connection> mConnections;
};

int main(int argc, char *argv[])
{
    {
        MyClass mc;

        sig(1.2, 10);
    }

    std::cout << "**********************" << std::endl;

    sig(1.2, 11);  // empty signal

    std::cout << "**********************" << std::endl;

    int i = 10;

    auto lambda = [&i](double d, int i) { std::cout << "in lambda: " << d << " " << i << std::endl; return 10; };
    sig.Bind(lambda, 1, 3.3, 0);

    sig.Bind([i](double d, int i) mutable -> int { std::cout << "in temp lambda: " << d << " " << i << std::endl; return 1; }, 2);

    sig(/* [](int i) -> bool { return i == 10; }, */ 1.20, 10);


    return 0;
}