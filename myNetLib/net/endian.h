#ifndef MYNETLIB_NET_ENDIAN_H
#define MYNETLIB_NET_ENDIAN_H
#include <cstdint>

inline uint16_t hostToNetwork16(uint16_t val){
    return (val << 8) | (val >> 8);
}

inline uint32_t hostToNetwork32(uint32_t val){
    val = ((val << 8) & 0xFF00FF00UL) | ((val >> 8) & 0x00FF00FFUL);
    return (val << 16) | (val >> 16);
}

inline uint64_t hostToNetwork64(uint64_t val){
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

inline uint16_t networkToHost16(uint16_t val){
    return hostToNetwork16(val);
}

inline uint32_t networkToHost32(uint32_t val){
    return hostToNetwork32(val);
}

inline uint64_t networkToHost64(uint64_t val){
    return hostToNetwork64(val);
}


#endif// MYNETLIB_NET_ENDIAN_H