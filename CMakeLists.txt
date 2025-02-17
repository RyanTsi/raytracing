cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Renderer)

set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/external)
set(glfw3_DIR ${PROJECT_SOURCE_DIR}/external/GLFW/lib/cmake/glfw3)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Assimp REQUIRED)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/glad)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/stb_image)

link_directories(
    ${PROJECT_SOURCE_DIR}/external/Assimp/lib
    ${PROJECT_SOURCE_DIR}/external/glfw/lib
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external/glad/include
    ${PROJECT_SOURCE_DIR}/external/stb_image/include
    ${PROJECT_SOURCE_DIR}/external/GLFW/include
    ${Assimp_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/shaders/shaderManager.cpp
    src/camera.cpp
    src/model.cpp
    src/material.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw3.lib
    glad
    stb_image
    glm::glm
    ${ASSIMP_LIBRARIES}
)

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

# copy .dll
file(GLOB ASSIMP_DLLS "${PROJECT_SOURCE_DIR}/external/Assimp/bin/*.dll")

foreach(DLL_FILE IN LISTS ASSIMP_DLLS)
    get_filename_component(DLL_NAME "${DLL_FILE}" NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/${DLL_NAME}
        COMMENT "Copying Assimp DLL ${DLL_NAME}"
    )
endforeach()