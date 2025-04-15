# Projects Settings
cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4514")
if(MSVC)
    # Disable specific warnings
    target_compile_options(${TARGET} PRIVATE 
        /wd4464  # Relative include path warning
        /wd4820  # Padding added after data member
        /wd4365  # Signed/unsigned mismatch
        /wd4244  # Conversion from 'double' to 'float', possible loss of data
        /wd5267  # Deprecated implicit assignment operator
        /wd5045  # Spectre mitigation
    )
endif()
project (RayTracing)

set(TARGET rtrt)

# The main executable
add_executable(${TARGET}
    src/main.cpp
    src/camera.cpp
    src/camera.h
    src/tools.cpp
    src/tools.h
    src/AABB.cpp
    src/AABB.h
    src/BVH.cpp
    src/BVH.h
    src/modelManager/light.cpp
    src/modelManager/light.h
    src/modelManager/material.cpp
    src/modelManager/material.h
    src/modelManager/mesh.cpp
    src/modelManager/mesh.h
    src/modelManager/scene.cpp
    src/modelManager/scene.h
    src/shaderManager/shader.cpp
    src/shaderManager/shader.h
    src/shaderManager/SSBO.cpp
    src/shaderManager/SSBO.h
)

set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 17)
# target_compile_options(${TARGET} PRIVATE -Wall)

add_subdirectory(lib/glad EXCLUDE_FROM_ALL)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/imgui EXCLUDE_FROM_ALL)
add_subdirectory(lib/stb_image EXCLUDE_FROM_ALL)
add_subdirectory(lib/assimp EXCLUDE_FROM_ALL)


target_include_directories(${TARGET} 
    PRIVATE lib/glad/include 
    lib/imgui/backends
    lib/assimp/include
)

target_link_libraries(${TARGET}
    PRIVATE glfw
    PRIVATE glm
    PRIVATE glad
    PRIVATE IMGUI
    PRIVATE stb_image
    PRIVATE assimp
)

target_include_directories(${TARGET}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/modelManager
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/shaderManager
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
)
