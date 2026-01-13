cmake_minimum_required(VERSION 3.15)
project(zstdBmpDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 查找依赖
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# 添加opencv
set(OpenCV_DIR $ENV{OPENCV455_DIR}) #这里配置了环境变量没有找到 暂时临时写绝对路径
set(OpenCV_DIR E:/Environment/OpenCV455/build/install)
#message(STATUS "opencvPath: $ENV{OPENCV455_DIR}")
message(STATUS "opencvPath: ${OpenCV_DIR}")
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
    message(STATUS "opencvIncludeDir: ${OpenCV_INCLUDE_DIRS}")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR CMAKE_SYSTEM_NAME MATCHES "Linux")
        list(APPEND LINK_LIBS ${OpenCV_LIBS})
    else()
        list(APPEND LINK_LIBS "opencv_world")
    endif()
endif()

# 设置项目路径
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ZSTD_COMPRESS_DIR "${PROJECT_ROOT}/zstdBmpCompress")
set(ZSTD_LIB_DIR "${PROJECT_ROOT}/lib")
set(ZSTD_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../zstdLib")

# 包含目录
include_directories(
        ${PROJECT_ROOT}
        ${ZSTD_COMPRESS_DIR}
        ${Qt5Core_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${ZSTD_INCLUDE}
)

# 链接目录
link_directories(${ZSTD_LIB_DIR})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

# release调试生成pdb
if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# 自动收集所有源文件
file(GLOB HEADER_FILES "*.h")
file(GLOB SOURCE_FILES "*.cpp")

# 打印找到的文件
message(STATUS "找到的头文件:")
foreach(header ${HEADER_FILES})
    message(STATUS "  ${header}")
endforeach()

message(STATUS "找到的源文件:")
foreach(source ${SOURCE_FILES})
    message(STATUS "  ${source}")
endforeach()

# 创建可执行文件 - 自动包含所有源文件
add_executable(zstdBmpDemo
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

# 链接库
target_link_libraries(zstdBmpDemo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        ${OpenCV_LIBS}
        zstdBmpCompressor
)