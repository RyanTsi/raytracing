# Projects Settings
cmake_minimum_required (VERSION 3.22)

project (RayTracing)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    # The bundled GLM still declares CMake 3.6 policies.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

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
    src/model/textures.cpp
    src/model/textures.h
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
target_compile_definitions(${TARGET} PRIVATE RTRT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(MSVC)
    target_compile_options(${TARGET} PRIVATE /W3 /utf-8)
endif()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MMD_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
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
if(WIN32)
    target_link_libraries(${TARGET} PRIVATE shell32)
endif()

target_include_directories(${TARGET}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/shader
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
)

include(CTest)
if(BUILD_TESTING)
    add_executable(core_tests tests/core_tests.cpp src/BVH.cpp src/AABB.cpp src/camera.cpp)
    target_compile_features(core_tests PRIVATE cxx_std_17)
    target_include_directories(core_tests PRIVATE src lib/glm)
    add_test(NAME core_contracts COMMAND core_tests)
    add_test(NAME gpu_smoke COMMAND rtrt --self-test --width 96 --height 64)
endif()

