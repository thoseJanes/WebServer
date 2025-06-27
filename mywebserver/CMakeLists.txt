# 自动收集源文件
aux_source_directory(redis SOURCE_FILES)
aux_source_directory(http SOURCE_FILES)
aux_source_directory(mysql SOURCE_FILES)
aux_source_directory(websocket SOURCE_FILES)
# 添加可执行文件
# add_executable(MyExecutable ${SOURCE_FILES})
add_library(mywebserver ${SOURCE_FILES})
target_link_libraries(mywebserver mynetlib)



list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
find_package(MySQL REQUIRED)
target_include_directories(mywebserver PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(mywebserver ${MYSQL_LIB})

find_package(OpenSSL REQUIRED)
#set(OpenSSL_LIB /usr/local/lib/libopenssl.so)
message(STATUS "Variable MY_VAR is ${OPENSSL_SSL_LIBRARY}")

set(HIREDIS_INCLUDE_DIR /usr/local/include/hiredis/include)
set(HIREDIS_LIBRARY /usr/local/lib/libhiredis.so)


add_executable(testProgram ./test/httpServerTest.cpp)
target_link_libraries(testProgram mywebserver)
target_link_libraries(testProgram ${OPENSSL_SSL_LIBRARY})
target_link_libraries(testProgram ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(testProgram ${MYSQL_LIB})
target_link_libraries(testProgram ${HIREDIS_LIBRARY})

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

