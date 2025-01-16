project(glad)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(glad STATIC ./src/gl.c)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)