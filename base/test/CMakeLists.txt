# add_executable(logger_test loggerTest.cpp)
# target_link_libraries(logger_test )

cmake_minimum_required(VERSION 3.10)
project(MyApp)

# 方式1：直接指定路径（推荐）
find_library(WEBSERVER_BASE_LIB webserver_base PATHS /usr/local/lib)

# 方式2：使用 find_package（需库提供 Config.cmake 文件，此处假设未提供）
# find_package(webserver_base REQUIRED)

# 创建可执行文件
add_executable(my_app loggerTest.cpp)

# 链接库和头文件
target_link_libraries(my_app ${WEBSERVER_BASE_LIB})
message(STATUS "${WEBSERVER_BASE_LIB}")
# target_link_libraries(my_app 
#     PRIVATE 
#     ${WEBSERVER_BASE_LIB} 
#     pthread 
#     rt
# )