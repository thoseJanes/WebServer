#ifndef WEBSERVER_TIME_DATE_H
#define WEBSERVER_TIME_DATE_H

namespace webserver{

struct YearMonthDay{
    int year;
    int month;
    int day;
};

class TimeStamp;
class Date{
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



struct DateTime{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};



#define DATE_OPERATOR_GENERATOR(op) \
inline bool operator op (const Date& d1, const Date& d2){ \
    return d1.julianDay() op d2.julianDay(); \
}

DATE_OPERATOR_GENERATOR(>)
DATE_OPERATOR_GENERATOR(>=)
DATE_OPERATOR_GENERATOR(<)
DATE_OPERATOR_GENERATOR(<=)
DATE_OPERATOR_GENERATOR(==)
DATE_OPERATOR_GENERATOR(!=)
#undef DATE_OPERATOR_GENERATOR


}

#endif
