cmake_minimum_required(VERSION 3.10)
project(MyProject)


# 设置编译器标志以生成调试信息
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 设置链接器标志以加载动态符号表
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
#add_subdirectory(test)
# set(SOURCE_FILES
#   file1.cc
#   file2.cc
#   )
# 自动收集源文件
aux_source_directory(common SOURCE_FILES)
aux_source_directory(logging SOURCE_FILES)
aux_source_directory(process SOURCE_FILES)
aux_source_directory(time SOURCE_FILES)
aux_source_directory(event SOURCE_FILES)
aux_source_directory(net SOURCE_FILES)
aux_source_directory(poller SOURCE_FILES)
aux_source_directory(http SOURCE_FILES)
# 添加可执行文件
# add_executable(MyExecutable ${SOURCE_FILES})
add_library(webserver_base ${SOURCE_FILES})
target_link_libraries(webserver_base pthread rt)


add_executable(testProgram ./test/httpServerTest.cpp)
target_link_libraries(testProgram webserver_base)


# install(TARGETS webserver_base DESTINATION lib)
# #install(TARGETS muduo_base_cpp11 DESTINATION lib)

# file(GLOB HEADERS 
#     "common/*.h"
#     "logging/*.h"
#     "process/*.h"
#     "time/*.h"
#     "event/*.h"
#     "net/*.h"
#     "poller/*.h")
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

