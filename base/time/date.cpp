#include "date.h"
#include "timeStamp.h"

namespace webserver{

Date::Date(int year, int month, int day){
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    julianDay_ = day + (153*m + 2) / 5 + y*365 + y/4 - y/100 + y/400 - 32045;//格里历转儒略日，格里历为现行公历
}

const int Date::julianDay_1970_01_01 = Date(1970,1,1).julianDay();

YearMonthDay Date::yearMonthDay() const {
    int a = julianDay_ + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - ((b * 146097) / 4);
    int d = (4 * c + 3) / 1461;
    int e = c - ((1461 * d) / 4);
    int m = (5 * e + 2) / 153;
    YearMonthDay ymd;
    ymd.day = e - ((153 * m + 2) / 5) + 1;
    ymd.month = m + 3 - 12 * (m / 10);
    ymd.year = b * 100 + d - 4800 + (m / 10);
    return ymd;
}

// TimeStamp Date::toTimeStamp() const {
//     int64_t secondSinceEpoch = (julianDay_ - julianDay_1970_01_01) * TimeStamp::kSecondsPerDay;
//     return secondSinceEpoch*TimeStamp::kMicroSecondsPerSecond;
// }


    
}