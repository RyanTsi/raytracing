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
    src/model/light.cpp
    src/model/light.h
    src/model/material.cpp
    src/model/material.h
    src/model/mesh.cpp
    src/model/mesh.h
    src/model/scene.cpp
    src/model/scene.h
    src/shader/shader.cpp
    src/shader/shader.h
    src/shader/SSBO.cpp
    src/shader/SSBO.h
    # src/ui/uiManager.cpp
    # src/ui/uiManager.h
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
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
    lib/imgui
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
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/shader
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
)

