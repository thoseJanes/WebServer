# 自动收集源文件
aux_source_directory(event SOURCE_FILES)
aux_source_directory(net SOURCE_FILES)
aux_source_directory(poller SOURCE_FILES)

# 添加可执行文件
add_library(mynetlib ${SOURCE_FILES})
target_link_libraries(mynetlib pthread rt)
target_link_libraries(mynetlib mynetbase)
