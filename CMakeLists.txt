cmake_minimum_required(VERSION 3.20)
project(PopcornBattle VERSION 1.0.0 LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================
# 依赖查找
# ============================================================

# SDL2
find_package(SDL2 REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# OpenCV (用于摄像头采集)
find_package(OpenCV REQUIRED COMPONENTS core videoio imgproc highgui)

# ONNX Runtime (用于姿态检测)
# 首先检查是否手动指定了路径
if(ONNXRUNTIME_ROOT)
    message(STATUS "Using manual ONNX Runtime path: ${ONNXRUNTIME_ROOT}")
    find_library(ONNXRUNTIME_LIB onnxruntime HINTS ${ONNXRUNTIME_ROOT}/lib)
    find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h HINTS ${ONNXRUNTIME_ROOT}/include)
    if(ONNXRUNTIME_LIB AND ONNXRUNTIME_INCLUDE)
        message(STATUS "Found ONNX Runtime library: ${ONNXRUNTIME_LIB}")
        message(STATUS "Found ONNX Runtime include: ${ONNXRUNTIME_INCLUDE}")
        set(ONNXRUNTIME_FOUND TRUE)
    else()
        message(WARNING "ONNX Runtime not found at ${ONNXRUNTIME_ROOT}")
        set(ONNXRUNTIME_FOUND FALSE)
    endif()
else()
    # 尝试通过 vcpkg 或系统查找
    find_package(onnxruntime CONFIG QUIET)
    if(NOT onnxruntime_FOUND)
        find_library(ONNXRUNTIME_LIB onnxruntime)
        find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h)
        if(ONNXRUNTIME_LIB AND ONNXRUNTIME_INCLUDE)
            message(STATUS "Found ONNX Runtime: ${ONNXRUNTIME_LIB}")
            set(ONNXRUNTIME_FOUND TRUE)
        else()
            message(WARNING "ONNX Runtime not found. Pose detection will be disabled.")
            set(ONNXRUNTIME_FOUND FALSE)
        endif()
    endif()
endif()

# GLEW (Windows 需要)
if(WIN32)
    find_package(GLEW REQUIRED)
endif()

# ============================================================
# 源文件
# ============================================================

set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Renderer.cpp
    src/camera/CameraCapture.cpp
    src/detection/PoseDetector.cpp
    src/detection/GestureDetector.cpp
    src/game/GameEngine.cpp
    src/game/CollisionSystem.cpp
)

set(HEADERS
    src/core/Application.h
    src/core/Window.h
    src/core/Renderer.h
    src/camera/CameraCapture.h
    src/detection/PoseDetector.h
    src/detection/GestureDetector.h
    src/game/GameEngine.h
    src/game/CollisionSystem.h
    src/game/FallingItem.h
)

# ============================================================
# 可执行文件
# ============================================================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    ${OpenCV_LIBS}
)

# ONNX Runtime
if(onnxruntime_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime::onnxruntime)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_ONNXRUNTIME)
    message(STATUS "ONNX Runtime enabled via vcpkg")
elseif(ONNXRUNTIME_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ONNXRUNTIME_LIB})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_ONNXRUNTIME)
    message(STATUS "ONNX Runtime enabled via manual path")
endif()

# Windows 额外链接 GLEW
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)
endif()

# ============================================================
# 平台特定配置
# ============================================================

if(APPLE)
    # macOS 特定设置
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    # Windows 特定设置
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
endif()

# ============================================================
# 复制资源文件（如果存在）
# ============================================================

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    )
endif()

# ============================================================
# 安装配置
# ============================================================

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "  Popcorn Battle - Native C++ Version")
message(STATUS "============================================================")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SDL2: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  OpenCV: ${OpenCV_VERSION}")
if(onnxruntime_FOUND OR ONNXRUNTIME_FOUND)
    message(STATUS "  ONNX Runtime: Enabled")
else()
    message(STATUS "  ONNX Runtime: Disabled (pose detection unavailable)")
endif()
message(STATUS "============================================================")
message(STATUS "")
