#ifndef WEBSERVER_COMMON_FORMAT_H
#define WEBSERVER_COMMON_FORMAT_H

#include <string.h>

namespace webserver{
namespace detail{
    template<typename T, int DIGIT = 10>
    size_t formatInteger(char* buf, T i);

    template<typename T, int DIGIT = 10>
    inline constexpr size_t maxFormatSize();

    template<typename T, int DIGIT = 10> 
    inline size_t maxFormatSize<T, DIGIT>(int precision);
}
}

#endif