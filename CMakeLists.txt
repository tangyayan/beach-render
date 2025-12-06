cmake_minimum_required(VERSION 3.15)
project(MyGLFWImGuiProject) 
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置编译器模式（Debug/Release）
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 查找 OpenGL
find_package(OpenGL REQUIRED)
# 下载并配置 GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip
)
FetchContent_MakeAvailable(glfw)
# 下载并配置 ImGui
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.4.zip
)
FetchContent_MakeAvailable(imgui)
# 添加 ImGui 源文件
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
set(IMGUI_HEADERS
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_internal.h
    ${imgui_SOURCE_DIR}/imconfig.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h
)
# 创建可执行文件
add_executable(${PROJECT_NAME} 
    "src/main.cpp"
    ${IMGUI_SOURCES}
    ${IMGUI_HEADERS}
 "src/glad.c" "src/obj.vs" "src/obj.fs" "include/myinclude/camera.h")
# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include/glm
    include
)
# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
)
# 在 Windows 上链接额外的库
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32.lib
    )
endif()
# 设置编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GL3W
)
# 复制 DLL 文件（Windows）
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:glfw>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
