cmake_minimum_required(VERSION 3.15)
project(zstdBmpCompressor C CXX)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DBMP_COMPRESSOR_EXPORTS)

# 定义变量
set(INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SRC_FILES "*.cpp")

# 添加三方库
# 添加qt{需要配置环境变量}
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(Qt5_DIR $ENV{QT_GCC_DIR})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(Qt5_DIR $ENV{QT_MSVC_DIR})
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
SET(QT_LIBS Qt5::Core Qt5::Widgets Qt5::Gui)
list(APPEND LINK_LIBS ${QT_LIBS})

message(STATUS "qtPath:${Qt5_DIR}")

# 添加opencv
set(OpenCV_DIR $ENV{OPENCV455_DIR}) #这里配置了环境变量没有找到 暂时临时写绝对路径
set(OpenCV_DIR E:/Environment/OpenCV455/build/install)
#message(STATUS "opencvPath: $ENV{OPENCV455_DIR}")
message(STATUS "opencvPath: ${OpenCV_DIR}")
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})  # 修复：OpenCV_INCLUDE_DIR -> OpenCV_INCLUDE_DIRS
    message(STATUS "opencvIncludeDir: ${OpenCV_INCLUDE_DIRS}")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR CMAKE_SYSTEM_NAME MATCHES "Linux")  # 修复：GUN -> GNU
        list(APPEND LINK_LIBS ${OpenCV_LIBS})
    else()
        list(APPEND LINK_LIBS "opencv_world")
    endif()
endif()

# 收集 Zstd
set(ZSTD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zstdLib")
file(GLOB_RECURSE ZSTD_SOURCES
        "${ZSTD_DIR}/common/*.c"
        "${ZSTD_DIR}/compress/*.c"
        "${ZSTD_DIR}/decompress/*.c"
)

include_directories(
        "${ZSTD_DIR}"
        "${ZSTD_DIR}/common"
        "${ZSTD_DIR}/compress"
        "${ZSTD_DIR}/decompress"
)

# 结束添加三方库

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# 生成动态库
add_library(zstdBmpCompressor SHARED
        zstdBmpCompressor.cpp
        ${ZSTD_SOURCES}
)

# 设置调试版本城市的可执行文件后缀包含"d"
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d)

# release调试生成pdb
if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# 头文件包含路径
target_include_directories(zstdBmpCompressor PRIVATE
        ${OpenCV_INCLUDE_DIRS}  # 添加OpenCV头文件包含
        "${ZSTD_DIR}/common"
        "${ZSTD_DIR}/compress"
        "${ZSTD_DIR}/decompress"
)

# 添加Qt5的头文件包含（通过target_link_libraries会自动包含）
target_link_libraries(zstdBmpCompressor PRIVATE
        ${QT_LIBS}
        ${LINK_LIBS}
        Threads::Threads
)

# 链接多线程库（移动到上面的target_link_libraries中）
find_package(Threads REQUIRED)

# 打印信息
message(STATUS "===========================================")
message(STATUS "开始配置 zstdBmpCompressor 项目")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "源码目录: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "构建目录: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "生成器: ${CMAKE_GENERATOR}")

message(STATUS "包含目录:")
foreach(dir ${INC_DIRS})
    message(STATUS "  - ${dir}")
endforeach()

message(STATUS "源文件:")
foreach(src ${SRC_FILES})
    message(STATUS "  - ${src}")
endforeach()

message(STATUS "三方库:")
foreach(lib ${LINK_LIBS})
    message(STATUS "  - ${lib}")
endforeach()

message(STATUS "===========================================")