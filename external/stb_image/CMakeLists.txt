project(stb_image)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(stb_image STATIC ./stb_image.cpp)

target_include_directories(stb_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)