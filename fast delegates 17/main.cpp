#include "delegate.hpp"
#include <iostream>

class MyClass
{
public:
    MyClass(int i) : i(i) {}
    int MemberFunction(double d) { std::cout << "in member function" << std::endl; return int(++i * d); }
    int ConstMemberFunction(double d) const { std::cout << "in const member function" << std::endl; return int(i * d); }
    int operator()(double d) { std::cout << "in overloaded function call operator" << std::endl; return (int)(++i + d); }
    int operator()(double d) const { std::cout << "in const overloaded function call operator" << std::endl; return (int)(i + d); }
    static int StaticMemberFunction(double d) { std::cout << "in static member function" << std::endl; return int(10 + d); }
private:
    int i;
};

DELEGATE_RET_ONE_PARAM(MyDelegate, int, double);
MyDelegate d1, d2, d3, d4, d5, d6, d7, d8;

MULTICAST_DELEGATE_RET_ONE_PARAM(MyMultiDelegate, int, double);
MyMultiDelegate md1;

int main(int argc, char **argv)
{
    MyClass mc(10);

    try 
    {
        d1.Invoke(0.2);
    }
    catch (DelegateNotBoundException exc)
    {
        std::cout << "delegate not bound!" << std::endl;
    }

    d1.Bind<&MyClass::MemberFunction>(mc);
    d1(11.0);

    d2.Bind<&MyClass::ConstMemberFunction>(mc);
    d2(1.0);

    MyClass const cmc(8);

    //d3.Bind<&MyClass::MemberFunction>(cmc);
    //d3(1.1);

    d3.Bind<&MyClass::ConstMemberFunction>(cmc);
    d3(1.1);

    d4.Bind<&MyClass::StaticMemberFunction>();
    d4(1.2);

    d5.Bind(mc);
    d5(1.3);

    d6.Bind(cmc);
    d6(1.5);

    int i = 10;
    auto lambda = [&i](double d) { std::cout << "in lvalue lambda" << std::endl; return i * d; };
    d7.Bind(lambda);
    d7(1.5);

    d8.Bind([i](double d) { std::cout << "in rvalue lambda" << std::endl; return i * d; });
    d8(1.5);

    std::cout << "******************** multicast delegate *******************" << std::endl;

    md1.Bind<&MyClass::MemberFunction>(mc);
    md1.Bind<&MyClass::ConstMemberFunction>(mc);
    md1.Bind<&MyClass::ConstMemberFunction>(cmc);
    md1.Bind<&MyClass::StaticMemberFunction>();
    md1.Bind(mc);
    md1.Bind(cmc);
    md1.Bind(lambda);
    md1.Bind([&i](double d){ std::cout << "in rvalue lambda" << std::endl; return i * d; });
    
    md1(1.20);

    return 0;
}