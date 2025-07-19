#include <memory>
#include <string>
#include <any>

using namespace std;
class Test : enable_shared_from_this<Test>{
public:
    void func(string str){printf("string_view:%s\n", str.data());};
    void func(string& str) const {printf("const string_view:%s\n", str.data());}
};

int main(){

    //不同类型的共享指针能否指向同一个对象，共享其计数
    Test test;
    shared_ptr<Test> testPtr(&test);
    printf("%ld\n", testPtr.use_count());

    {
        shared_ptr<void> testPtr2(testPtr);
        printf("%ld\n", testPtr.use_count());
    }
        

    {
        shared_ptr<void> testPtr2(&test);
        printf("%ld\n", testPtr.use_count());
    }

    {
        shared_ptr<std::any> testPtr2(make_shared<any>(&test));
        printf("%ld\n", testPtr.use_count());
    }


    





}