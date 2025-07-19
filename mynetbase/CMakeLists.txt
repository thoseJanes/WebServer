# 自动收集源文件
aux_source_directory(common SOURCE_FILES)
aux_source_directory(logging SOURCE_FILES)
aux_source_directory(process SOURCE_FILES)
aux_source_directory(time SOURCE_FILES)

add_library(mynetbase ${SOURCE_FILES})
target_link_libraries(mynetbase pthread rt)
