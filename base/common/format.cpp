#include "format.h"




using namespace std;
namespace webserver{
namespace detail{
    //const char numbers[] = "FEDCBA9876543210123456789ABCDEF";
    //const char* zero = numbers + 15;
    // const char constexpr zero[] = "0123456789ABCDEF";
    // template<typename T, int DIGIT>
    // size_t formatInteger(char* buf, T i)

    //获取整型的最大表示长度
    //函数不允许偏特化
    // template<typename T, int DIGIT>
    // constexpr size_t maxFormatSize(){
    //     if constexpr (DIGIT == 10){
    //         static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
    //         return std::numeric_limits<T>::digits10 + 1;//负号长度
    //     }else if constexpr (DIGIT == 2){
    //         static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
    //         return std::numeric_limits<T>::digits + 1;//负号长度
    //     }else if constexpr (DIGIT == 16){
    //         static_assert(is_integral<T>::value, "maxFormatSize: T must be an integer type.");
    //         return (std::numeric_limits<T>::digits-1)/4 + 1 + 1;//截断 + 负号。
    //     }else{
    //         static_assert(false, "maxFormatSize: can't get format size.");
    //         return 0;
    //     }
    // }

    




    // //获取浮点类型的最大表示长度
    // template<typename T, int DIGIT> 
    // size_t maxFormatSize(int precision)

}
}