#ifndef WEBSERVER_COMMON_FORMAT_H
#define WEBSERVER_COMMON_FORMAT_H
#include <type_traits>//is_integral
#include <string.h>
#include <limits>
#include <algorithm>//reverse

using namespace std;
namespace webserver{
namespace detail{
    

    template<typename T, int DIGIT = 10>
    size_t formatInteger(char* buf, T i){
        static const char constexpr zero[] = "0123456789ABCDEF";
        static_assert(*zero == '0', "zero pointer error.");
        static_assert(DIGIT <= 16 && DIGIT > 1);
        static_assert(is_integral<T>::value, "formatInteger: T must be an integer type.");
        bool negative = (i<0);
        if(negative){
            *buf++ = '-';
            i = -i;
        }
        
        int len = 0;
        while(i!=0){
            int n = i%DIGIT;
            i /= DIGIT;
            buf[len++] = zero[n];
        }
        
        buf[len] = '\0';
        std::reverse(buf, buf+len);

        return len + negative;
    }

    template<typename T, int DIGIT = 10>
    constexpr size_t maxFormatSize(){//调用maxFormatSize的地方必须能看到maxFormatSize的定义
        if constexpr (DIGIT == 10){
            static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
            return std::numeric_limits<T>::digits10 + 1;//负号长度
        }else if constexpr (DIGIT == 2){
            static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
            return std::numeric_limits<T>::digits + 1;//负号长度
        }else if constexpr (DIGIT == 16){
            static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
            return (std::numeric_limits<T>::digits-1)/4 + 1 + 1;//截断 + 负号。
        }else{
            static_assert(false, "maxFormatSize: can't get format size.");
            return 0;
        }
    }

    //获取某个具体整数的表示长度
    template<typename T, int DIGIT>
    size_t integerLength(T i){
        char* buf = new char[maxFormatSize<T, DIGIT>()];
        auto len = formatInteger<T, DIGIT>(buf, i);
        delete buf;
        return len;
    }


    template<typename T, int DIGIT = 10> 
    size_t maxFormatSize(int precision){
        if constexpr (DIGIT == 10){
            static_assert(is_floating_point<T>::value, "maxFormatSize: T must be an floating point type."); 
            static int expLen = 1 + 1 + integerLength(numeric_limits<T>::max_exponent10);//指数符号+指数符号e+指数最长
            return 2+precision+expLen;//负号长度+小数点长度
        }else{
            static_assert(false, "maxFormatSize: can't get format size.");
            return 0;//负号长度+小数点长度
        }
    }

}
}

#endif