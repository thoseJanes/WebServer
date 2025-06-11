#include "strStream.h"
#include <algorithm>//reverse
#include <assert.h>
#include <limits>//numeric_limits
#include <stdint.h>//uintptr_t
#include <stdio.h>//snprintf
#include <stdarg.h>//va_list 变参数


namespace webserver{

// constexpr char* StrStream::Buffer::truncatedFlag = "...(truncated)";
// constexpr int StrStream::Buffer::truncatedLen = sizeof(truncatedFlag);



#define LEFT_SHIFT_GENERATOR(tp) template<int SIZE>\
StrStream<SIZE>& StrStream<SIZE>::operator<<(tp i){ \
    formatInteger<tp>(i);\
    return *this;\
}

LEFT_SHIFT_GENERATOR(short);
LEFT_SHIFT_GENERATOR(int);
LEFT_SHIFT_GENERATOR(long);
LEFT_SHIFT_GENERATOR(long long);
LEFT_SHIFT_GENERATOR(unsigned short);
LEFT_SHIFT_GENERATOR(unsigned int);
LEFT_SHIFT_GENERATOR(unsigned long);
LEFT_SHIFT_GENERATOR(unsigned long long);
#undef LEFT_SHIFT_GENERATOR

template<int SIZE>
StrStream<SIZE>& StrStream<SIZE>::operator<<(const void* ptr){
    auto v = reinterpret_cast<uintptr_t>(ptr);//reinterpret_cast负责指针和整数间的互相转换。
    buf_.append("0x", 2);
    formatInteger<uintptr_t, 16>(v);
    return *this;
}

template<int SIZE>
int StrStream<SIZE>::precision_ = 12;


#define LEFT_SHIFT_GENERATOR(fmt, tp) template<int SIZE>\
StrStream<SIZE>& StrStream<SIZE>::operator<<(tp val){ \
    int offset = snprintf(buf_.current(), buf_.availBytes()+1, (fmt), precision_, val); \
    if(offset <= buf_.availBytes()){ \
        buf_.moveWriteIndex(offset); \
    }else{ \
        buf_.setTruncatedFlag(); \
    } \
    return *this; \
}

LEFT_SHIFT_GENERATOR("%.*g", float);
LEFT_SHIFT_GENERATOR("%.*g", double);
LEFT_SHIFT_GENERATOR("%.*Lg", long double);
#undef LEFT_SHIFT_GENERATOR
/*
StrStream& StrStream::operator<<(long double val){
    //static int precision = 12;//会成为有效数字,g规则：如果小于10^-4或大于10^precision则采用科学表示法%.[precision]e，否则采用%.[precision]f
    //最大长度：1(负号)+1(小数点)+precision(有效数字位数)+1(e)+1(指数正负号)+k(double指数最大十进制位数，)
    //对于float？, 对于double为precision+7, 对于long double为precision+8, 
    int maxSize = precision_ + 8;
    int offset = snprintf(buf_.current(), buf_.availBytes()+1, "%.*g", precision_, val);
    if(offset <= buf_.availBytes()){
        buf_.moveWriteIndex(offset);
    }else{
        buf_.setTruncatedFlag();
    }
    return *this;
}
*/
template<int SIZE>
StrStream<SIZE>& StrStream<SIZE>::operator<<(const char* str){
    buf_.append(str, strlen(str));//append strlen不会在最后添加末尾字符。
    return *this;
}

template<int SIZE>
StrStream<SIZE>& StrStream<SIZE>::operator<<(char chr){
    buf_.append(&chr, 1);
    return *this;
}

template<int SIZE>
StrStream<SIZE>& StrStream<SIZE>::withFormat(const char* fmt, ...){
    int avail = buf_.availBytes();
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf_.current(), avail+1, fmt, args);
    va_end(args);
    if(len<=avail){
        buf_.moveWriteIndex(len);
    }else{//超出范围了,即为avail+1
        buf_.setTruncatedFlag();
    }
    return *this;
}

template<int SIZE>
StrStream<SIZE>& StrStream<SIZE>::withFormat(int maxlen, const char* fmt, ...){
    int avail = buf_.availBytes();
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf_.current(), min<int>(avail+1, maxlen), fmt, args);
    va_end(args);
    if(len<=avail){
        buf_.moveWriteIndex(len);
    }else{//超出范围了,即为avail+1
        buf_.setTruncatedFlag();
    }
    return *this;
}


Fmt::Fmt(const char* fmt, ...){//如果截断了怎么办？
    va_list args;
    va_start(args, fmt);
    len = vsnprintf(buf_, 32, fmt, args);
    assert(len < 32);//确保没有截断
    va_end(args);
}


template class StrStream<4000>;
template class StrStream<512>;

}