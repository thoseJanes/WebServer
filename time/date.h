#ifndef WEBSERVER_TIME_DATE_H
#define WEBSERVER_TIME_DATE_H
#include <boost/operators.hpp>

namespace webserver{

struct YearMonthDay{
    int year;
    int month;
    int day;
};

class TimeStamp;
class Date : public boost::equality_comparable<Date>, 
            public boost::less_than_comparable<Date>{
public:
    Date():julianDay_(0){};
    Date(int julianDay):julianDay_(julianDay){};
    Date(int year, int month, int day);
    int year() const {return yearMonthDay().year;}
    int month() const {return yearMonthDay().month;}
    int day() const {return yearMonthDay().day;}
    //ISO: [1..7]->[Mon, Tue, Wed, Thu, Fri, Sat, Sun]
    int weekday() const {return julianDay_/7+1;}
    int julianDay() const {return julianDay_;}
    YearMonthDay yearMonthDay() const;
    //TimeStamp toTimeStamp() const;

    static const int julianDay_1970_01_01;
private:
    int julianDay_;
};

inline bool operator<(const Date& d1, const Date& d2){
    return d1.julianDay()<d2.julianDay();
}
inline bool operator==(const Date& d1, const Date& d2){
    return d1.julianDay()==d2.julianDay();
}

struct DateTime{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

}

#endif
