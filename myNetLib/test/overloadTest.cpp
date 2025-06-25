#include <functional>
#include <stdio.h>
#include <string>
using namespace std;
class Test{
public:
    void func(string str){printf("string_view:%s\n", str.data());};
    void func(string& str) const {printf("const string_view:%s\n", str.data());}
};

//注意以下三个函数相互冲突。不构成重载。
int testFunc(int i){return 0;};
//int testFunc(const int i){return 0;};
//const int testFunc(int i){return 0;}


int main(){
    {
        function<void(int i)> funci = [](int i){};
        function<void()> func = bind(funci, 1);
    }

    //bind重载函数时的使用方法（以及bind的参数的存储方式。）
    {
        string_view testView;
        Test testClass;
        function<void()> func;
        {
            string test = "1234";
            string_view testView = test;
            func = bind<void(Test::*)(string&) const>(&Test::func, &testClass, test);
        }
        
        func();
        
        //printf("%s\n", testView.data());

    }
}