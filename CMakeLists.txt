cmake_minimum_required(VERSION 3.10)
project(MyProject)


# 设置编译器标志以生成调试信息
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 设置链接器标志以加载动态符号表
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
#add_subdirectory(test)
# set(SOURCE_FILES
#   file1.cc
#   file2.cc
#   )
# 自动收集源文件
add_subdirectory(mybase)
add_subdirectory(mynetlib)
add_subdirectory(mywebserver)



# list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
# find_package(MySQL REQUIRED)
# target_include_directories(webserver_base PRIVATE ${MYSQL_INCLUDE_DIR})
# target_link_libraries(webserver_base ${MYSQL_LIB})

# set(HIREDIS_INCLUDE_DIR /usr/local/include/hiredis/include)
# set(HIREDIS_LIBRARY /usr/local/lib/libhiredis.so)

# add_executable(testProgram ./test/httpServerTest.cpp)
# target_link_libraries(testProgram webserver_base)
# target_link_libraries(testProgram ${MYSQL_LIB})
# target_link_libraries(testProgram ${HIREDIS_LIBRARY})

# 以下操作会将webserver安装到/usr/local/include目录下。

# install(TARGETS webserver_base DESTINATION lib)
# #install(TARGETS muduo_base_cpp11 DESTINATION lib)

# file(GLOB HEADERS 
#     "common/*.h"
#     "logging/*.h"
#     "process/*.h"
#     "time/*.h"
#     "event/*.h"
#     "net/*.h"
#     "poller/*.h"
#     "http/*.h"
#     "mysql/*.h"
#     )
# # install(FILES ${HEADERS} DESTINATION include/webserver/base)
# # 为每个文件生成安装路径
# foreach(file ${HEADERS})
#     # 提取相对路径（保留目录结构）
#     file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
#     # 获取目标路径
#     get_filename_component(target_path ${relative_file} DIRECTORY)
#     message(STATUS "Variable MY_VAR is ${file}")
#     # 安装文件并保留目录结构
#     install(FILES ${file} DESTINATION include/webserver/base/${target_path})
#     message(STATUS "installed path is include/webserver/base/${target_path}")
# endforeach()

