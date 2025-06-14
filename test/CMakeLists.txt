# add_executable(logger_test loggerTest.cpp)
# target_link_libraries(logger_test )

cmake_minimum_required(VERSION 3.10)
project(test)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/Modules")
find_package(MySQL REQUIRED)

find_package(hiredis REQUIRED)
# 方式1：直接指定路径（推荐）
find_library(WEBSERVER_BASE_LIB webserver_base PATHS /usr/local/lib)


add_executable(loggerTest loggerTest.cpp)
target_link_libraries(loggerTest ${WEBSERVER_BASE_LIB})


add_executable(processTest processTest.cpp)
target_link_libraries(processTest ${WEBSERVER_BASE_LIB})


add_executable(threadTest threadTest.cpp)
target_link_libraries(threadTest ${WEBSERVER_BASE_LIB})

add_executable(eventLoopTest eventLoopTest.cpp)
target_link_libraries(eventLoopTest ${WEBSERVER_BASE_LIB})

add_executable(tcpServerTest tcpServerTest.cpp)
target_link_libraries(tcpServerTest ${WEBSERVER_BASE_LIB})


add_executable(mysqlTest mysqlTest.cpp)
target_link_libraries(mysqlTest ${WEBSERVER_BASE_LIB})
target_link_libraries(mysqlTest ${MYSQL_LIB})

add_executable(httpServerTest httpServerTest.cpp)
target_link_libraries(httpServerTest ${WEBSERVER_BASE_LIB})
target_link_libraries(httpServerTest ${MYSQL_LIB})

add_executable(redisTest redisTest.cpp)
target_link_libraries(redisTest ${WEBSERVER_BASE_LIB})
target_link_libraries(redisTest hiredis::hiredis)

