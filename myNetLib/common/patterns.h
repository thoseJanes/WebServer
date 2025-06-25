#ifndef MYNETLIB_COMMON_PATTERN_H
#define MYNETLIB_COMMON_PATTERN_H

namespace mynetlib{

class Noncopyable{
public:
    //根据 C++ 的标准，赋值运算符的返回类型通常是类的引用（T&），但标准也允许返回其他类型，包括 void。这使得赋值运算符的重载在某些情况下可以返回不同的类型，而不会导致重复定义错误。
    //void operator=(const noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
    Noncopyable(const Noncopyable&) = delete;
protected://声明为protected意味着不能直接创建 noncopyable 类的对象，但可以派生子类并实例化子类对象
    Noncopyable() = default;
    ~Noncopyable() = default;
};


template<typename T>
class Singleton:Noncopyable{
public:
    static T& instance(){
        static T singleton;
        return singleton;
    }
private:
    Singleton(){};
};

}

#endif