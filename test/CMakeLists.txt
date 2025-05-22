# add_executable(logger_test loggerTest.cpp)
# target_link_libraries(logger_test )

cmake_minimum_required(VERSION 3.10)
project(MyApp)

# 方式1：直接指定路径（推荐）
find_library(WEBSERVER_BASE_LIB webserver_base PATHS /usr/local/lib)


add_executable(loggerTest loggerTest.cpp)
target_link_libraries(loggerTest ${WEBSERVER_BASE_LIB})


add_executable(processTest processTest.cpp)
target_link_libraries(processTest ${WEBSERVER_BASE_LIB})