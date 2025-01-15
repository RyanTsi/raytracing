project(glad)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(glad STATIC ./src/gl.c)

# 设置公共包含目录，以便主项目能访问glad的头文件
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)