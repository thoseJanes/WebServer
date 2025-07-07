# 自动收集源文件
aux_source_directory(redis SOURCE_FILES)
aux_source_directory(http SOURCE_FILES)
aux_source_directory(mysql SOURCE_FILES)
aux_source_directory(websocket SOURCE_FILES)
# 添加可执行文件
# add_executable(MyExecutable ${SOURCE_FILES})
add_library(mynet ${SOURCE_FILES})
target_link_libraries(mynet mynetlib)



list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
find_package(MySQL REQUIRED)
target_include_directories(mynet PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(mynet ${MYSQL_LIB})

find_package(OpenSSL REQUIRED)
#set(OpenSSL_LIB /usr/local/lib/libopenssl.so)
message(STATUS "Variable MY_VAR is ${OPENSSL_SSL_LIBRARY}")

set(HIREDIS_INCLUDE_DIR /usr/local/include/hiredis/include)
set(HIREDIS_LIBRARY /usr/local/lib/libhiredis.so)


add_executable(testProgram ./test/httpServerTest.cpp)
target_link_libraries(testProgram mynet)
target_link_libraries(testProgram ${OPENSSL_SSL_LIBRARY})
target_link_libraries(testProgram ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(testProgram ${MYSQL_LIB})
target_link_libraries(testProgram ${HIREDIS_LIBRARY})





