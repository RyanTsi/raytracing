cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Renderer)

set(GLAD_SOURCES_DIR ${PROJECT_SOURCE_DIR}/external/glad)
set(STB_IMAGE_SOURCES_DIR ${PROJECT_SOURCE_DIR}/external/stb_image)

set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(${GLAD_SOURCES_DIR})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${GLAD_SOURCES_DIR}/include
    ${STB_IMAGE_SOURCES_DIR}/include
    /usr/local/include
)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/shaders/shaderManager.cpp
    src/camera.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm)

# glsl copy
file(GLOB_RECURSE SHADER_FILES RELATIVE ${CMAKE_SOURCE_DIR}/src/shaders ${CMAKE_SOURCE_DIR}/src/shaders/*.glsl)

foreach(SHADER_FILE IN LISTS SHADER_FILES)
    get_filename_component(SHADER_PATH "${SHADER_FILE}" PATH)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/shaders/${SHADER_PATH}")

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/src/shaders/${SHADER_FILE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/src/shaders/${SHADER_FILE}" "${CMAKE_BINARY_DIR}/src/shaders/${SHADER_FILE}"
        MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/src/shaders/${SHADER_FILE}"
        COMMENT "Copying shader ${SHADER_FILE}"
        VERBATIM
    )

    list(APPEND SHADERS_COPIED "${CMAKE_BINARY_DIR}/src/shaders/${SHADER_FILE}")
endforeach()

add_custom_target(CopyShaders ALL DEPENDS ${SHADERS_COPIED})