# zstdBmpCompressor - 图像压缩库

![](doc\pic\1.png)

## 项目概述

ZstdBmpCompressor 是一个基于 Zstandard (Zstd) 压缩算法的跨平台图像压缩库，支持多种图像格式（BMP、PNG、JPEG等）的压缩和解压缩操作。该项目使用 C++17 开发，集成了 Qt、OpenCV 和 Zstd 库，提供高效的图像处理能力。



## 功能特性

### 核心功能

- **图像压缩**: 支持单个图像和批量图像的压缩

- **图像解压缩**: 支持压缩数据的解压和图像恢复

- **多格式支持**: BMP、PNG、JPEG 等常见图像格式

- **高性能**: 利用 Zstd 算法提供快速的压缩和解压缩

- **多线程支持**: 可配置线程数以优化性能

  

### 输入输出支持

- **输入类型**:

  - 文件路径（std::string 和 QString）
  - QImage 对象
  - cv::Mat 对象
  - 原始字节数据

- **输出类型**:

  - 压缩后的字节数据

  - QImage 对象

  - cv::Mat 对象

  - 保存到文件

    

### 批量处理

- 支持整个文件夹的图像批量压缩

- 支持批量解压缩到指定目录

- 自动识别图像文件和压缩文件

  

## 技术架构

### 依赖库

- **Zstandard**: 提供核心压缩算法

- **Qt5**: 提供 GUI 支持和 QImage 处理（Core、Gui、Widgets 模块）

- **OpenCV**: 提供计算机视觉功能和 cv::Mat 处理

- **C++17**: 现代 C++ 特性支持

  

### 编译配置

- **构建系统**: CMake 3.15+

- **编译器支持**: GCC (Linux) 和 MSVC (Windows)

- **平台支持**: Windows 和 Linux

  

## 项目结构

复制

```
zstdBmpCompressor/
├── CMakeLists.txt          # 项目构建配置
├── zstdBmpCompressor.h     # 头文件 - 类声明和接口
├── zstdBmpCompressor.cpp   # 源文件 - 实现代码
├── zstdLib/                # Zstd 库源码
│   ├── common/
│   ├── compress/
│   └── decompress/
├── bin/                    # 可执行文件输出目录
└── lib/                    # 库文件输出目录
```



## 核心类说明

### ImageCompressor 类
