#ifndef WEBSERVER_LOGGING_LOGSTREAM_H
#define WEBSERVER_LOGGING_LOGSTREAM_H
//#include <memory>
#include <string.h>
#include "../common/patterns.h"
#include "../common/format.h"
#include <string>
#include <string_view>







namespace webserver{
namespace detail{


template<int SIZE>
class LogBuffer: public Noncopyable{
public:
    LogBuffer():cur_(data_){}
    void append(const char* buf, int len){
        if(availBytes() >= len){
            memcpy(current(), buf, len);
            moveWriteIndex(len);
        }else{
            memcpy(current(), buf, availBytes());
            setTruncatedFlag();//或者此时应当立即flush到文件中？
        }
    }

    //注意，设置之后availBytes会变成0.
    void setTruncatedFlag(){
        //assert(false);
        //if(data_.writableBytes() >= truncatedLen){
            cur_ = data_ + SIZE;
            memcpy(cur_, truncatedFlag, truncatedLen);
            cur_ = data_ + SIZE + truncatedLen;
        //}
    }
    void moveWriteIndex(int offset){cur_ += offset;}
    char* current(){return cur_;}
    int writtenBytes() const {
        return static_cast<int>(cur_ - data_);
    }
    int availBytes() const {
        return std::max<int>(SIZE - writtenBytes(), 0);
    }

    const char* data() const {return data_;}
    void reset(){cur_ = data_;}
private:
    int writableBytes(){
        return SIZE - writtenBytes() +  truncatedLen;
    }
    static constexpr char truncatedFlag[] = "...(truncated)\n";
    static constexpr int truncatedLen = sizeof(truncatedFlag) - 1;
    char* cur_;
    char data_[SIZE + truncatedLen + 1];//留一个位置用于snprintf是否超出长度判断。

};


}

class Fmt: public Noncopyable{
public:
    Fmt(const char* fmt, ...);
    char buf_[32];
    int len;
};

using namespace std;
class LogStream: public Noncopyable{
    typedef detail::LogBuffer<4000> Buffer;
    typedef LogStream self;
public:
    LogStream() = default;
    ~LogStream() = default;
    self& operator<<(short);
    self& operator<<(int);
    self& operator<<(long);
    self& operator<<(long long);
    self& operator<<(unsigned short);
    self& operator<<(unsigned int);
    self& operator<<(unsigned long);
    self& operator<<(unsigned long long);
    // #define LEFT_SHIFT_GENERATOR(tp) \
    //     self& operator<<(tp);
    // LEFT_SHIFT_GENERATOR(short);
    // LEFT_SHIFT_GENERATOR(int);
    // LEFT_SHIFT_GENERATOR(long);
    // LEFT_SHIFT_GENERATOR(long long);
    // LEFT_SHIFT_GENERATOR(unsigned short);
    // LEFT_SHIFT_GENERATOR(unsigned int);
    // LEFT_SHIFT_GENERATOR(unsigned long);
    // LEFT_SHIFT_GENERATOR(unsigned long long);
    // #undef LEFT_SHIFT_GENERATOR
    self& operator<<(long double);
    self& operator<<(float val);//{
    //     *this << static_cast<long double>(val);
    //     return *this;
    // }
    self& operator<<(double val);//{
    //     *this << static_cast<long double>(val);
    //     return *this;
    // }

    self& operator<<(const void*);
    
    self& operator<<(char);
    self& operator<<(const char*);
    self& operator<<(unsigned char chr){
        *this << static_cast<char>(chr);
        return *this;
    }
    self& operator<<(const unsigned char* str){
        *this << reinterpret_cast<const char*>(str);
        return *this;
    }
    
    self& operator<<(const string& str){
        buf_.append(str.c_str(), str.size());
        return *this;
    }
    self& operator<<(const string_view& str){
        buf_.append(str.data(), str.size());
        return *this;
    }

    self& operator<<(Fmt& str){
        buf_.append(str.buf_, str.len);
        return *this;
    }
    
    //使用方法类似这样？：LOG_INFO << a << ", " << b ("a%d", d) << k
    //也太奇怪了。想向里面插入点东西，分隔一下，譬如 LOG_INFO << a << ", " << b << Fmt("a%d", d) << k
    //通过operator<<(Fmt& str)凑成LOG_INFO << a << ", " << b << Fmt()("a%d", d) << k的形式？
    //由于()的运算优先级高于<<，因此不能这样用！
    
    //此外，在这种函数上使用变参数模板可能会产生很多特化版本，得不偿失？
    self& withFormat(const char* fmt, ...);
    self& withFormat(int maxlen, const char* fmt, ...);
    const Buffer& buffer(){return buf_;}


    static int getPrecision(){
        return precision_;
    }

private:
    template<typename T, int DIGIT = 10>
    void formatInteger(T i){
        if(buf_.availBytes() >= detail::maxFormatSize<T, DIGIT>()){
            buf_.moveWriteIndex(detail::formatInteger<T, DIGIT>(buf_.current(), i));
        }else{
            buf_.setTruncatedFlag();
        }
    }

    Buffer buf_;
    static int precision_;
};

//struct Fmt{};




}



#endif