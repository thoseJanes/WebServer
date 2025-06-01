#include <execinfo.h>
#include <string>
#include <algorithm>
#include <cxxabi.h>
#include <memory>
//注意：编译时需要生成调试符号（-g）以及导出动态符号表（-rdynamic)，否则backtrace_symbols输出无法显示函数名称。
//执行如下命令运行即可：
//cd "/home/eleanor-taylor/work/C++/webServer/test/" && g++ -g -rdynamic testStackTrace.cpp -o testStackTrace && "/home/eleanor-taylor/work/C++/webServer/test/"testStackTrace
int testFunc(int a, std::string b){
    int maxFrameNum = 200;
    void* frames[maxFrameNum];
    int frameNum = backtrace(frames, maxFrameNum);
    char** strings = backtrace_symbols(frames, frameNum);

    std::string stack;
    bool demangle = true;
    
    size_t len = 256;
    char* demangled = demangle?static_cast<char*>(malloc(len*sizeof(char))):nullptr;
    for(int i=0;i<frameNum;i++){
        if(demangle){
            char* nameStart = strings[i];
            char* nameEnd = strings[i];
            for(char* p = strings[i];*p!='\0';p++){
                if(*p == '('){
                    nameStart = p;
                }else if(*p == '+'){
                    nameEnd = p;
                }
            }
            *nameEnd = '\0';
            int status = 0;
            //如果放入的内存大小不足，函数会重新malloc一块内存（并释放原内存），ret是返回的内存地址。
            //这里len是放入的内存块的大小。
            auto ret = abi::__cxa_demangle(nameStart+1, demangled, &len, &status);
            *nameEnd = '+';
            printf("len:%ld, status:%d, offset:%ld\n", len, status, ret - demangled);
            if(status == 0){
                demangled = ret;
                stack.append(strings[i], nameStart+1);
                stack.append(demangled);
                stack.append(nameEnd);
                stack += "\n";
            }
        }else{
            stack.append(strings[i]);
            stack += "\n";
        }
        
    }
    free(demangled);
    free(strings);
    printf("\n%s\n", stack.c_str());
    return a;
}

int testFunc2(){
    testFunc(1, "232");
    return 0;
}

int main(){
    testFunc2();
}

