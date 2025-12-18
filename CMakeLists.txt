cmake_minimum_required(VERSION 3.15)
project(MyGLFWImGuiProject) 
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置编译器模式（Debug/Release）
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
if (MSVC)
    add_compile_options("/utf-8")
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
# 下载并配置 Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1  # 使用稳定版本
)
# Assimp 编译选项（减少不需要的功能以加快编译）
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# 只启用需要的格式（加快编译速度）
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
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

# 收集 background 目录中的所有源文件
file(GLOB BACKGROUND_SOURCES 
    "${CMAKE_SOURCE_DIR}/background/*.cpp"
)

file(GLOB BACKGROUND_HEADERS 
    "${CMAKE_SOURCE_DIR}/background/*.h"
)

# 收集 utils 目录中的所有源文件
file(GLOB UTILS_SOURCES 
    "${CMAKE_SOURCE_DIR}/utils/*.cpp"
)

file(GLOB UTILS_HEADERS 
    "${CMAKE_SOURCE_DIR}/utils/*.h"
)

# 收集 model 目录中的所有源文件
file(GLOB MODEL_SOURCES
    "${CMAKE_SOURCE_DIR}/model/*.cpp"
)

file(GLOB MODEL_HEADERS
    "${CMAKE_SOURCE_DIR}/model/*.h"
)

# 打印找到的文件（用于调试）
message(STATUS "Found background sources: ${BACKGROUND_SOURCES}")
message(STATUS "Found background headers: ${BACKGROUND_HEADERS}")
message(STATUS "Found utils sources: ${UTILS_SOURCES}")
message(STATUS "Found utils headers: ${UTILS_HEADERS}")


# 创建可执行文件
add_executable(${PROJECT_NAME} 
    "src/main.cpp"
    "src/glad.c"
    ${IMGUI_SOURCES}
    ${IMGUI_HEADERS}
    ${BACKGROUND_SOURCES}
    ${BACKGROUND_HEADERS}
    ${UTILS_SOURCES}
    ${UTILS_HEADERS}
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include/glm
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/myinclude
    ${CMAKE_SOURCE_DIR}/include/stb_image
    ${CMAKE_SOURCE_DIR}/background          # 添加 background 目录
    ${CMAKE_SOURCE_DIR}/utils               # 添加 utils 目录
    ${CMAKE_SOURCE_DIR}/model               # 添加 model 目录
    ${assimp_SOURCE_DIR}/include            # Assimp 源码头文件
    ${assimp_BINARY_DIR}/include            # Assimp 生成的头文件
)
# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    assimp  # 链接 Assimp
)
# 在 Windows 上链接额外的库
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32.lib
    )
    # 复制 Assimp DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
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