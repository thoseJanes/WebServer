# 自动收集源文件
aux_source_directory(redis SOURCE_FILES)
aux_source_directory(http SOURCE_FILES)
aux_source_directory(mysql SOURCE_FILES)
aux_source_directory(websocket SOURCE_FILES)
# 添加可执行文件
# add_executable(MyExecutable ${SOURCE_FILES})
add_library(mywebserver ${SOURCE_FILES})



target_link_libraries(mywebserver mynetlib)
target_link_libraries(mywebserver mynetbase)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
find_package(MySQL REQUIRED)
find_package(OpenSSL REQUIRED)
set(HIREDIS_INCLUDE_DIR /usr/local/include/hiredis/include)
set(HIREDIS_LIBRARY /usr/local/lib/libhiredis.so)
target_link_libraries(mywebserver ${MYSQL_LIB})
target_link_libraries(mywebserver ${OPENSSL_SSL_LIBRARY})
target_link_libraries(mywebserver ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(mywebserver ${HIREDIS_LIBRARY})


add_executable(testProgram ./test/httpServerTest.cpp)
target_link_libraries(testProgram mywebserver)







