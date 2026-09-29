# 数独求解器（C++ / Win32 GUI）

一个带图形界面的数独暴力破解器。手动填数，一键求解。基于 Win32 API，无第三方依赖，编译即用。

## 特性

- **Win32 图形界面**：鼠标点击选中格子，键盘 `1-9` 填数，方向键移动，`Backspace` 清空
- **一键求解**：回溯 + MRV（最少候选数优先）启发式，配合位掩码判重
- **颜色区分**：黑色为题目给定数字，蓝色为程序求解结果
- **模块化设计**：算法与界面完全解耦，`sudoku_core` 零 Windows 依赖，可单独复用
- **零依赖**：仅用 Win32 API，无需 Qt / MFC

## 目录结构

```text
project/
├── main.cpp            程序入口
├── main_window.h/.cpp  窗口类注册与消息处理
├── app.h/.cpp          应用状态与操作
├── renderer.h/.cpp     绘制逻辑
└── sudoku_core.h/.cpp  求解算法（无 Windows 依赖）
```

## 求解算法

- **位掩码判重**：每行、每列、每宫各用一个 9 位整数记录已出现的数字，判断候选数只需一次位运算
- **MRV 启发式**：每步优先处理候选数最少的空格，大幅剪枝
- **冲突预检查**：求解前先校验题目本身是否矛盾

实测在难题上，MRV 能把搜索节点数降低几个数量级。

## 编译

### MinGW-w64 / g++

```bash
g++ -O2 -std=c++17 -mwindows \
    main.cpp main_window.cpp app.cpp renderer.cpp sudoku_core.cpp \
    -o sudoku_gui.exe -lgdi32 -luser32
```

### CMake（推荐，Windows / VS 通用）

```cmake
cmake_minimum_required(VERSION 3.20)
project(SudokuGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sudoku_gui WIN32
    main.cpp main_window.cpp app.cpp renderer.cpp sudoku_core.cpp)

target_link_libraries(sudoku_gui PRIVATE user32 gdi32)

if (MSVC)
    target_compile_options(sudoku_gui PRIVATE /utf-8)
endif()
```

用 VS 打开该文件夹即可一键构建（`WIN32` 关键字会自动设置 `/SUBSYSTEM:WINDOWS`）。

## Visual Studio 导入提示

如果手动建项目导入源码，注意以下几点：

- **PCH 要统一**：项目开着预编译头就必须每个 `.cpp` 第一行 `#include "pch.h"`；没有 `pch.h` 就在属性里关掉 PCH。二者必须同时成立或不成立，否则会报 `C2065` / `C1083`。
- **子系统**：链接器 → 系统 → 子系统 = **窗口 (/SUBSYSTEM:WINDOWS)**
- **所有 `.cpp` 都要加入项目**：拷贝文件 ≠ 加入项目，缺文件会在链接时报 `LNK2019`
- **头文件自包含**：`renderer.h` 和 `main_window.h` 自己包含 `windows.h`，不依赖调用方的包含顺序
- **编码**：源文件保存为 UTF-8 with BOM，避免中文注释乱码

## 使用

1. 启动程序
2. 点击格子，按 `1-9` 填数；按 `Backspace` 清空
3. 点「求解」得到答案（蓝色数字）

内置一道示例题目，点「示例题目」即可载入。


## License

MIT

---
