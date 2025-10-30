# External Sort Implementation

SET(EXTERNAL_SORT_SOURCES
    tuple_serializer.cpp
    temp_file_manager.cpp
    external_sorter.cpp
)

# 将源文件添加到父目录的变量中
SET(EXTERNAL_SORT_SOURCES ${EXTERNAL_SORT_SOURCES} PARENT_SCOPE)

