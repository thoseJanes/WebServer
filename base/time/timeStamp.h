#ifndef WEBSERVER_TIME_TIMESTAMP_H
#define WEBSERVER_TIME_TIMESTAMP_H
#include <boost/operators.hpp>
#include <sys/time.h>

namespace webserver{

class TimeStamp : public boost::equality_comparable<TimeStamp>, 
                public boost::less_than_comparable<TimeStamp>{
public:
    TimeStamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){}
    static const int kMicroSecondsPerSecond = 1000*1000;
    static const int kSecondsPerDay = 24*60*60;
    int64_t getMicroSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
    
    static TimeStamp now(){
        timeval time;
        gettimeofday(&time, NULL);
        return TimeStamp((int64_t)time.tv_sec * kMicroSecondsPerSecond + time.tv_usec);
    }
private:
    int64_t microSecondsSinceEpoch_;
};


inline bool operator<(const TimeStamp& t1, const TimeStamp& t2){
    return t1.getMicroSecondsSinceEpoch() < t2.getMicroSecondsSinceEpoch();
}
inline bool operator==(const TimeStamp& t1, const TimeStamp& t2){
    return t1.getMicroSecondsSinceEpoch() == t2.getMicroSecondsSinceEpoch();
}


}


#endif
