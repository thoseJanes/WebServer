#include <stdio.h>
#include <vector>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "../../mynetbase/common/patterns.h"
using namespace mynetlib;
class TestClass{
    friend Singleton<TestClass>;
private:
    TestClass(){};
};

template<typename... Args>
void pass_to_snprintf(char* buffer, size_t size, const char* format, Args&&... args) {
    snprintf(buffer, size, format, std::forward<Args>(args)...);
}

void getFile(const char file[]){
    //printf("%ld\n", sizeof (file));
    //printf("%d\n", (int)strlen(file));
}

void printfFormatTest(){
    char buf[255]; int bufSize = 255; int formatSize; int precision;
    printf("%ld", sizeof buf);
    std::vector<double> testNums = {-123.456789*pow(10, 300), 0.000012345, 0.00012345, 123456789.0, 12345678.0, 1234567.0, 123456.0, 12345.0, 1234.0};

    precision = 6;
    for(int i=0;i<testNums.size();i++){
        formatSize = snprintf(buf, bufSize,"%.*g", precision, testNums[i]);//snprintf输出长度不包括最后的‘\0'符号
        printf("precision:%d, out:%s, formatSize:%d\n", precision, buf, formatSize);
        if(formatSize > precision + 7){
            printf("error!");
            exit(0);
        }
    }

    printf("%05d\n", 50);
    printf("%5d\n", 50);
}

using namespace std;
int main() {

    //pass_to_snprintf(buf, 255, "presdfadfascision:%d, formatSize:%d\n", precision, formatSize);
    //printf("%s", buf);
    //printf("%ld\n", sizeof __FILE__);
    //getFile(__FILE__);

    //TestClass& t = Singleton<TestClass>::instance();


    return 0;
}