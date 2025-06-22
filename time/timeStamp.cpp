#include "timeStamp.h"

using namespace webserver;

string TimeStamp::toFormattedString(bool showMicroSeconds) const {
    time_t seconds = microSecondsSinceEpoch_/kMicroSecondsPerSecond;
    tm tmstruct;
    gmtime_r(&seconds, &tmstruct);
    
    char buf[64];
    if(showMicroSeconds){
        snprintf(buf, sizeof(buf), "%04d%02d%02d %02d:%02d:%02d.%06dZ", 
                tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, 
                tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec, 
                static_cast<int>(microSecondsSinceEpoch_%kMicroSecondsPerSecond));
    }else{
        snprintf(buf, sizeof(buf), "%04d%02d%02d %02d:%02d:%02d", 
                tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, 
                tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
    }
    return buf;
}