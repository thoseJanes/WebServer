# add_executable(logger_test loggerTest.cpp)
# target_link_libraries(logger_test )

cmake_minimum_required(VERSION 3.10)
project(test)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/Modules")
find_package(MySQL REQUIRED)

find_package(hiredis REQUIRED)
# 方式1：直接指定路径（推荐）
find_library(MYNET_LIB mynetlib PATHS /usr/local/lib)

find_package(openssl REQUIRED)


add_executable(mysqlTest mysqlTest.cpp)
target_link_libraries(mysqlTest ${MYNET_LIB})
target_link_libraries(mysqlTest ${MYSQL_LIB})

add_executable(httpServerTest httpServerTest.cpp)
target_link_libraries(httpServerTest ${MYNET_LIB})
target_link_libraries(httpServerTest ${MYSQL_LIB})

add_executable(redisTest redisTest.cpp)
target_link_libraries(redisTest ${MYNET_LIB})
target_link_libraries(redisTest hiredis::hiredis)

add_executable(base64Test base64Test.cpp)
target_link_libraries(base64Test openssl)

