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

# 自动收集源文件
add_subdirectory(mynetbase)
add_subdirectory(mynetlib)
add_subdirectory(mywebserver)




# 以下操作会将webserver安装到/usr/local/include&lib目录下。

install(TARGETS mynetbase DESTINATION lib)
install(TARGETS mynetlib DESTINATION lib)
install(TARGETS mywebserver DESTINATION lib)


file(GLOB_RECURSE HEADERS 
    "mynetbase/*.h"
    "mynetlib/*.h"
    "mywebserver/*.h"
    )

message(STATUS "HEADERS is ${HEADERS}")
# 为每个文件生成安装路径
foreach(file ${HEADERS})
    # 提取相对路径（保留目录结构）
    file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    # 获取目标路径
    get_filename_component(target_path ${relative_file} DIRECTORY)
    
    # 安装文件并保留目录结构
    install(FILES ${file} DESTINATION include/mynet/${target_path})
    message(STATUS "installed path is include/mynet/${target_path}")
endforeach()



