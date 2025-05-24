#ifndef WEBSERVER_TIME_TIMESTAMP_H
#define WEBSERVER_TIME_TIMESTAMP_H
#include <boost/operators.hpp>
#include <sys/time.h>
#include <string>
using namespace std;
namespace webserver{

class TimeStamp : public boost::equality_comparable<TimeStamp>, 
                public boost::less_than_comparable<TimeStamp>{
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
    string toFormattedString(bool microSeconds){
        time_t seconds = microSecondsSinceEpoch_/kMicroSecondsPerSecond;
        tm tmstruct;
        gmtime_r(&seconds, &tmstruct);
        
        char buf[32];
        if(microSeconds){
            snprintf(buf, sizeof(buf), "%04d%02d%02d %02d:%02d:%02d.%06dZ", 
                    tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, 
                    tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec, 
                    microSecondsSinceEpoch_%kMicroSecondsPerSecond);
        }else{
            snprintf(buf, sizeof(buf), "%04d%02d%02d %02d:%02d:%02d", 
                    tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, 
                    tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
        }
        return buf;
    }
    void add(int microSeconds){this->microSecondsSinceEpoch_ += microSeconds;}
private:
    int64_t microSecondsSinceEpoch_;
};

//offset = to - from
int64_t microSecondsOffset(TimeStamp from, TimeStamp to){
    return to.getMicroSecondsSinceEpoch() - from.getMicroSecondsSinceEpoch();
}

inline bool operator<(const TimeStamp& t1, const TimeStamp& t2){
    return t1.getMicroSecondsSinceEpoch() < t2.getMicroSecondsSinceEpoch();
}
inline bool operator==(const TimeStamp& t1, const TimeStamp& t2){
    return t1.getMicroSecondsSinceEpoch() == t2.getMicroSecondsSinceEpoch();
}


}


#endif
