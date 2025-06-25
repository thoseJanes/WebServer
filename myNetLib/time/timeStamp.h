#ifndef MYNETLIB_TIME_TIMESTAMP_H
#define MYNETLIB_TIME_TIMESTAMP_H
#include <sys/time.h>
#include <string>
#include <ctime>
#include <unistd.h>
using namespace std;
namespace mynetlib{

class TimeStamp{
public:
    explicit TimeStamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){}
    static const int kMicroSecondsPerSecond = 1000*1000;
    static const int kSecondsPerDay = 24*60*60;
    int64_t getMicroSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
    
    static TimeStamp now(){
        timeval time;
        gettimeofday(&time, NULL);
        return TimeStamp((int64_t)time.tv_sec * kMicroSecondsPerSecond + time.tv_usec);
    }
    string toFormattedString(bool showMicroSeconds = false) const;
    void add(int microSeconds){this->microSecondsSinceEpoch_ += microSeconds;}
    void swap(TimeStamp& that){
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }
private:
    int64_t microSecondsSinceEpoch_;
};

//offset = to - from
inline int64_t microSecondsOffset(TimeStamp from, TimeStamp to){
    return to.getMicroSecondsSinceEpoch() - from.getMicroSecondsSinceEpoch();
}

#define TIMESTAMP_OPERATOR_GENERATOR(op) \
inline bool operator op (const TimeStamp& t1, const TimeStamp& t2){ \
    return t1.getMicroSecondsSinceEpoch() op t2.getMicroSecondsSinceEpoch(); \
}

TIMESTAMP_OPERATOR_GENERATOR(>)
TIMESTAMP_OPERATOR_GENERATOR(>=)
TIMESTAMP_OPERATOR_GENERATOR(<)
TIMESTAMP_OPERATOR_GENERATOR(<=)
TIMESTAMP_OPERATOR_GENERATOR(==)
TIMESTAMP_OPERATOR_GENERATOR(!=)
#undef TIMESTAMP_OPERATOR_GENERATOR


}


#endif
