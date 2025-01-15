cmake_minimum_required(VERSION 3.10)
project(Renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GLAD_SOURCES_DIR ${PROJECT_SOURCE_DIR}/external/glad)

add_subdirectory(${GLAD_SOURCES_DIR})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${GLAD_SOURCES_DIR}/include
    /usr/local/include
)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)