#include <functional>

using namespace std;
class Test{
public:
    int func(int i){return 0;};
    int func(int i) const {return 0;}
};
int testFunc(int i){return 0;};

int main(){
    {
        function<void(int i)> funci = [](int i){};
        function<void()> func = bind(funci, 1);
    }
    {
        Test test;
        function<int()> func = bind(Test::func, test, 1);
        func = bind(testFunc, 1);
    }
}