# add_executable(logger_test loggerTest.cpp)
# target_link_libraries(logger_test )

cmake_minimum_required(VERSION 3.10)
project(test)


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

