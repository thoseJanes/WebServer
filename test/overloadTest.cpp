#include <functional>

using namespace std;
class Test{
public:
    int func(int i){return 0;};
    int func(int i) const {return 0;}
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
    {
    }
}