#include "format.h"
#include <limits>

namespace webserver{
namespace detail{
    //const char numbers[] = "FEDCBA9876543210123456789ABCDEF";
    //const char* zero = numbers + 15;
    const char zero[] = "0123456789ABCDEF";
    template<typename T, int DIGIT = 10>
    size_t formatInteger(char* buf, T i){
        assert(*zero == '0');
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
    inline constexpr size_t maxFormatSize(){
        static_assert(false, "maxFormatSize: can't get format size.");
        return 0;
    }

    //获取整型的最大表示长度
    template<typename T>
    inline constexpr size_t maxFormatSize<T, 10>(){
        static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
        return std::numeric_limits<T>::digits10 + 1;//负号长度
    }
    template<typename T>
    inline constexpr size_t maxFormatSize<T, 2>(){
        static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
        return std::numeric_limits<T>::digits + 1;//负号长度
    }
    template<typename T>
    inline constexpr size_t maxFormatSize<T, 16>(){
        static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
        return (std::numeric_limits<T>::digits-1)/4 + 1 + 1;//截断 + 负号。
    }

    //获取某个具体整数的表示长度
    template<typename T, int DIGIT = 10>
    size_t integerLength(T i){
        char* buf = new char[maxFormatSize<T, DIGIT>()];
        auto len = formatInteger<T, DIGIT>(buf, i);
        delete buf;
        return len;
    }

    //获取浮点类型的最大表示长度
    template<typename T, int DIGIT = 10> 
    inline size_t maxFormatSize<T, DIGIT>(int precision){
        static_assert(DIGIT==10, "maxFormatSize: can't get format DIGIT.");//只支持十进制。
        static_assert(is_floating_point<T>::value, "maxFormatSize: T must be an floating point type."); 
        static expLen = 1 + 1 + integerLength(numeric_limits<T>::max_exponent10);//指数符号+指数符号e+指数最长
        return 2+precision+expLen;//负号长度+小数点长度
    }
}
}