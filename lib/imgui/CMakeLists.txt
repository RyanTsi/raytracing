cmake_minimum_required (VERSION 3.20)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/)
add_library(IMGUI STATIC)
set_property(TARGET IMGUI PROPERTY CXX_STANDARD 17)

target_sources(IMGUI
                PRIVATE
                    ${IMGUI_DIR}/imgui_demo.cpp
                    ${IMGUI_DIR}/imgui_draw.cpp
                    ${IMGUI_DIR}/imgui_tables.cpp
                    ${IMGUI_DIR}/imgui_widgets.cpp
                    ${IMGUI_DIR}/imgui.cpp
                    # ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
                    # ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories( IMGUI
                            PUBLIC ${IMGUI_DIR}
                            PUBLIC ${IMGUI_DIR}/backends
                            PUBLIC ${SDL2_DIR}/include
)
