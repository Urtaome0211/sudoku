# 从零写一个 Windows 原生数独求解器：纯 Win32 + 位掩码回溯

> 本文带你用 **C++17 + 纯 Win32 API + GDI**，不依赖任何第三方库，从零实现一个可交互的 Windows 桌面数独求解器：点击格子选格、键盘输入数字、一键求解，还带防闪烁的双缓冲绘制。核心是一个**位掩码 + MRV 启发式 + 回溯**的求解引擎。完整源代码附在文末。

---

## 目录

1. [项目概览](#1-项目概览)
2. [架构设计](#2-架构设计)
3. [核心算法：位掩码 + 回溯](#3-核心算法位掩码--回溯)
4. [界面与交互：双缓冲绘制](#4-界面与交互双缓冲绘制)
5. [构建与运行](#5-构建与运行)
6. [总结](#6-总结)
7. [完整源代码](#7-完整源代码)

---

## 1. 项目概览

这是一个运行在 Windows 上的数独 GUI 程序，功能包括：

- **输入题目**：点击格子选中，按 `1–9`（或小键盘）输入数字，`Backspace` / `Delete` 清除；
- **加载示例**：一键载入内置样例题目；
- **一键求解**：自动求解并高亮显示答案；
- **防闪烁渲染**：双缓冲 GDI 绘制，3×3 宫用粗线分隔。

| 项目 | 内容 |
|------|------|
| 语言 / 标准 | C++17 |
| GUI 技术 | Win32 API + GDI（无任何第三方库） |
| 构建系统 | CMake |
| 目标产物 | `sudoku_gui.exe`（`WIN32` 子系统，无控制台窗口） |
| 入口函数 | `WinMain` |

代码按职责拆分为 6 个源文件，总计约 500 行，非常适合作为学习原生 Windows 编程和回溯算法的入门项目。

---

## 2. 架构设计

项目采用清晰的四层结构，依赖方向单向，算法层与 UI 完全解耦：

```
main.cpp             入口 + 消息循环
main_window.*        窗口注册/创建 + 消息分发（WM_*）
app.*                应用状态（棋盘、选中格、字体）+ 业务逻辑（单例）
renderer.*           GDI 纯绘制（棋盘/数字/高亮/提示）
sudoku_core.*        纯算法（校验 + 回溯求解），不依赖任何 UI 类型
```

依赖链为：

```
main_window ──► app ──► sudoku_core
    └──────────► renderer ──► app
```

`app` 模块通过 Meyers 单例 `app()` 持有全部运行时状态：

- `board_[9][9]`：当前盘面数字；
- `given_[9][9]`：是否为题目给定数字（用于区分「不可改的题面」与「用户/程序填入的值」）；
- `selR_ / selC_`：当前选中格坐标；
- `fontNum_ / fontUI_`：两种 GDI 字体。

---

## 3. 核心算法：位掩码 + 回溯

`sudoku_core.cpp` 是整个项目的技术亮点。求解器用 **位掩码** 表示数字占用情况，配合 **MRV 启发式** 和 **安全阀**，实现了高效且健壮的求解。

### 3.1 用位掩码表示占用

数独的约束是「每行、每列、每个 3×3 宫各含 1–9 各一次」。我们用三个数组分别记录行、列、宫的占用情况：

```cpp
int rm[9] = {};  // rm[r] 的第 d-1 位为 1，表示第 r 行已占用数字 d
int cm[9] = {};  // 列占用
int bm[9] = {};  // 宫占用，宫索引 = (r/3)*3 + c/3
```

数字 `d` 对应 `1 << (d-1)`。于是某格 `(r, c)` 的候选数字集合只需一步求出：

```cpp
int used  = rm[r] | cm[c] | bm[(r/3)*3 + c/3];
int avail = (~used) & 0x1FF;   // 低 9 位为 1 的位即候选数字
```

### 3.2 MRV 启发式：优先填「最难」的格子

朴素回溯是从左到右逐个空格尝试，效率很低。MRV（Minimum Remaining Values，最少剩余值）策略改为：**每次都选候选数最少的空格**先填，剪枝效果显著。实现如下：

```cpp
int bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
for (int r = 0; r < N; ++r) {
    for (int c = 0; c < N; ++c) {
        if (b[r][c]) continue;
        int avail = (~(rm[r] | cm[c] | bm[(r/3)*3 + c/3])) & 0x1FF;
        int cnt   = popcount9(avail);
        if (cnt == 0) return false;     // 无候选，此分支死路
        if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
    }
}
if (bestR == -1) return true;           // 没有空格，已解出
```

`popcount9` 是手写的 Brian Kernighan 计数法，避免依赖编译器内建：

```cpp
inline int popcount9(int x) {
    int n = 0;
    while (x) { x &= (x - 1); ++n; }
    return n;
}
```

### 3.3 安全阀

GUI 程序最怕病态输入卡死界面。求解器在递归中维护节点计数，超过 2000 万节点强制返回：

```cpp
if (++nodes > 20000000LL) return false;   // 安全阀
```

### 3.4 冲突校验

`solve` 前先 `validate`，用同一套位掩码逻辑做 O(81) 的冲突检测；`app::solveCurrent` 也会先校验，失败则用消息框提示而不破坏原盘面。

---

## 4. 界面与交互：双缓冲绘制

### 4.1 双缓冲消除闪烁

`WM_PAINT` 处理中，先在内存 DC 上完成全部绘制，再一次性 `BitBlt` 到屏幕：

```cpp
HDC     memDC  = CreateCompatibleDC(hdc);
HBITMAP memBmp = CreateCompatibleBitmap(hdc, CLIENT_W, CLIENT_H);
HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

renderer::draw(memDC);

BitBlt(hdc, 0, 0, CLIENT_W, CLIENT_H, memDC, 0, 0, SRCCOPY);

SelectObject(memDC, oldBmp);
DeleteObject(memBmp);
DeleteDC(memDC);
```

同时让 `WM_ERASEBKGND` 返回 1，禁用系统默认的背景擦除，二者配合即可彻底消除闪烁。

### 4.2 绘制顺序

`renderer::draw` 按固定图层顺序绘制：

1. 窗口背景（浅灰）
2. 棋盘白底
3. 选中格高亮（浅蓝）
4. 网格线（3×3 宫用 3px 粗线，其余 1px 细线）
5. 数字（题目给定为黑色、用户/程序填入为蓝色）
6. 顶部提示文字

### 4.3 键盘与鼠标交互

- `WM_LBUTTONDOWN`：由坐标反推 `(行, 列)`，选中格子；
- `WM_KEYDOWN`：方向键移动选中格，`1–9` / 小键盘输入数字，`Backspace` / `Delete` 清除；**对 `given_` 标记的题目格拒绝修改**，保证题面不被破坏。

三个按钮（清空 / 示例题目 / 求解）是子窗口 `BUTTON` 控件，处理完 `WM_COMMAND` 后调用 `SetFocus(hwnd)` 把焦点还给主窗口，确保键盘持续可用。

---

## 5. 构建与运行

环境：Windows + Visual Studio（或任何带 MSVC 的 CMake 环境）。

```powershell
cmake -S . -B out/build
cmake --build out/build --config Debug
```

生成的 `sudoku_gui.exe` 直接双击运行即可。

`CMakeLists.txt` 有两个关键点：

- `add_executable(sudoku_gui WIN32 ...)`：`WIN32` 关键字自动设置 `/SUBSYSTEM:WINDOWS`，无需手动改链接器属性；
- `/utf-8` 编译选项：强制源文件按 UTF-8 解析，避免中文注释乱码。

---

## 6. 总结

这个项目虽小，但覆盖了原生 Windows 编程的多个关键知识点：

- Win32 窗口注册、创建与消息循环；
- 子窗口控件（BUTTON）与 `WM_COMMAND` 分发；
- GDI 双缓冲绘制与字体/画笔/画刷的使用；
- 位掩码 + MRV 启发式 + 回溯的搜索算法；
- 单例模式与模块化分层设计。

如果要进一步扩展，可以考虑：区分「用户输入」与「程序求解」的颜色、内置多道题目、加入「提示下一步」功能、或引入随机题目生成器。

---

## 7. 完整源代码

以下为项目全部源码，按目录结构排列。将其保存为对应文件名，配合 `CMakeLists.txt` 即可编译运行。

### 7.1 `CMakeLists.txt`

```cpp
cmake_minimum_required(VERSION 3.20)
project(SudokuGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WIN32 关键字 = 自动设置 /SUBSYSTEM:WINDOWS，不用再手动改属性
add_executable(sudoku_gui WIN32
    main.cpp
    main_window.cpp
    app.cpp
    renderer.cpp
    sudoku_core.cpp
)

target_link_libraries(sudoku_gui PRIVATE user32 gdi32)

if (MSVC)
    # 强制 UTF-8，避免中文注释乱码
    target_compile_options(sudoku_gui PRIVATE /utf-8)
endif()
```

### 7.2 `main.cpp`

```cpp
#include <windows.h>
#include "main_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    main_window::registerClass(hInstance);

    if (!main_window::create(hInstance, nCmdShow))
        return 0;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
```

### 7.3 `main_window.h`

```cpp
#pragma once
#include <windows.h>

namespace main_window {

// 注册窗口类
void registerClass(HINSTANCE hInstance);

// 创建并显示主窗口，失败返回 nullptr
HWND create(HINSTANCE hInstance, int nCmdShow);

} // namespace main_window
```

### 7.4 `main_window.cpp`

```cpp
#include "main_window.h"
#include "app.h"
#include "renderer.h"

namespace {

const wchar_t* kClassName = L"SudokuWnd";

// ---------------- 子控件创建 ----------------
void onCreate(HWND hwnd)
{
    app().attach(hwnd);

    const int BW  = 120;
    const int BH  = 40;
    const int BY  = 620;
    const int GAP = 20;
    const int X0  = (CLIENT_W - (BW * 3 + GAP * 2)) / 2;

    struct ButtonDef {
        const wchar_t* text;
        int            id;
        bool           isDefault;
    } buttons[] = {
        { L"清空",     ID_BTN_CLEAR,  false },
        { L"示例题目", ID_BTN_SAMPLE, false },
        { L"求解",     ID_BTN_SOLVE,  true  },
    };

    for (int i = 0; i < 3; ++i) {
        DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
        if (buttons[i].isDefault) style |= BS_DEFPUSHBUTTON;

        CreateWindowW(L"BUTTON", buttons[i].text, style,
                      X0 + (BW + GAP) * i, BY, BW, BH,
                      hwnd, (HMENU)(INT_PTR)buttons[i].id,
                      nullptr, nullptr);
    }
}

// ---------------- 按钮响应 ----------------
void onCommand(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam)) {
    case ID_BTN_CLEAR:  app().clear();        break;
    case ID_BTN_SAMPLE: app().loadSample();   break;
    case ID_BTN_SOLVE:  app().solveCurrent(); break;
    }
    SetFocus(hwnd);          // 焦点还给主窗口，保证键盘可用
}

// ---------------- 双缓冲绘制 ----------------
void onPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    HDC     memDC  = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, CLIENT_W, CLIENT_H);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    renderer::draw(memDC);

    BitBlt(hdc, 0, 0, CLIENT_W, CLIENT_H, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

// ---------------- 窗口过程 ----------------
LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        onCreate(hwnd);
        return 0;

    case WM_COMMAND:
        onCommand(hwnd, wParam);
        return 0;

    case WM_LBUTTONDOWN:
        app().onLButtonDown((short)LOWORD(lParam), (short)HIWORD(lParam));
        return 0;

    case WM_KEYDOWN:
        app().onKeyDown(wParam);
        return 0;

    case WM_ERASEBKGND:
        return 1;               // 交给双缓冲擦除

    case WM_PAINT:
        onPaint(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

void main_window::registerClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;

    RegisterClassExW(&wc);
}

HWND main_window::create(HINSTANCE hInstance, int nCmdShow)
{
    RECT rc = { 0, 0, CLIENT_W, CLIENT_H };
    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowExW(0, kClassName, L"数独求解器", style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top,
                                nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return nullptr;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return hwnd;
}
```

### 7.5 `app.h`

```cpp
#pragma once
#include <windows.h>
#include "sudoku_core.h"

// ---- 界面常量 ----
constexpr int CELL_SIZE  = 60;
constexpr int BOARD_X    = 30;
constexpr int BOARD_Y    = 60;
constexpr int CLIENT_W   = 600;
constexpr int CLIENT_H   = 680;

// ---- 按钮 ID ----
constexpr int ID_BTN_CLEAR  = 101;
constexpr int ID_BTN_SAMPLE = 102;
constexpr int ID_BTN_SOLVE  = 103;

class SudokuApp {
public:
    SudokuApp();
    ~SudokuApp();

    void attach(HWND hwnd);
    HWND hwnd() const { return hwnd_; }

    int  at(int r, int c) const          { return board_[r][c]; }
    bool givenAt(int r, int c) const     { return given_[r][c]; }
    int  selRow() const                  { return selR_; }
    int  selCol() const                  { return selC_; }

    HFONT numberFont() const             { return fontNum_; }
    HFONT uiFont() const                 { return fontUI_;  }

    // 操作
    void clear();
    void loadSample();
    bool solveCurrent();
    void redraw();

    // 消息处理
    void onLButtonDown(int x, int y);
    void onKeyDown(WPARAM key);

private:
    void createFonts();
    void destroyFonts();

    HWND  hwnd_   = nullptr;
    int   board_[sudoku::N][sudoku::N] = {};
    bool  given_[sudoku::N][sudoku::N] = {};
    int   selR_   = -1;
    int   selC_   = -1;
    HFONT fontNum_ = nullptr;
    HFONT fontUI_  = nullptr;
};

// 全局单例
SudokuApp& app();
```

### 7.6 `app.cpp`

```cpp
#include "app.h"
#include <cstring>

namespace {

const char* kSample[9] = {
    "530070000",
    "600195000",
    "098000060",
    "800060003",
    "400803001",
    "700020006",
    "060000280",
    "000419005",
    "000080079"
};

} // namespace

SudokuApp::SudokuApp()  { createFonts(); }
SudokuApp::~SudokuApp() { destroyFonts(); }

void SudokuApp::attach(HWND hwnd) { hwnd_ = hwnd; }

void SudokuApp::createFonts()
{
    fontNum_ = CreateFontW(38, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    fontUI_ = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
}

void SudokuApp::destroyFonts()
{
    if (fontNum_) { DeleteObject(fontNum_); fontNum_ = nullptr; }
    if (fontUI_)  { DeleteObject(fontUI_);  fontUI_  = nullptr; }
}

void SudokuApp::clear()
{
    std::memset(board_, 0, sizeof(board_));
    std::memset(given_, 0, sizeof(given_));
    selR_ = selC_ = -1;
    redraw();
}

void SudokuApp::loadSample()
{
    for (int r = 0; r < sudoku::N; ++r)
        for (int c = 0; c < sudoku::N; ++c) {
            char ch = kSample[r][c];
            board_[r][c] = (ch == '0') ? 0 : (ch - '0');
            given_[r][c] = (ch != '0');
        }
    selR_ = selC_ = -1;
    redraw();
}

bool SudokuApp::solveCurrent()
{
    if (!sudoku::validate(board_)) {
        MessageBoxW(hwnd_,
            L"当前盘面存在冲突（同行/同列/同宫有重复数字），请先修正。",
            L"无法求解", MB_OK | MB_ICONWARNING);
        return false;
    }

    int tmp[sudoku::N][sudoku::N];
    std::memcpy(tmp, board_, sizeof(tmp));

    if (sudoku::solve(tmp)) {
        std::memcpy(board_, tmp, sizeof(board_));
        redraw();
        return true;
    }

    MessageBoxW(hwnd_, L"该数独无解。", L"提示", MB_OK | MB_ICONWARNING);
    return false;
}

void SudokuApp::redraw()
{
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void SudokuApp::onLButtonDown(int x, int y)
{
    int bx = x - BOARD_X;
    int by = y - BOARD_Y;
    if (bx < 0 || by < 0) return;

    int c = bx / CELL_SIZE;
    int r = by / CELL_SIZE;
    if (r < 0 || r >= sudoku::N || c < 0 || c >= sudoku::N) return;

    selR_ = r;
    selC_ = c;
    redraw();
}

void SudokuApp::onKeyDown(WPARAM key)
{
    if (selR_ < 0 || selC_ < 0) return;

    bool changed = false;
    switch (key) {
    case VK_LEFT:  if (selC_ > 0)               { --selC_; changed = true; } break;
    case VK_RIGHT: if (selC_ < sudoku::N - 1)   { ++selC_; changed = true; } break;
    case VK_UP:    if (selR_ > 0)               { --selR_; changed = true; } break;
    case VK_DOWN:  if (selR_ < sudoku::N - 1)   { ++selR_; changed = true; } break;

    case VK_BACK:
    case VK_DELETE:
        if (!given_[selR_][selC_] && board_[selR_][selC_] != 0) {
            board_[selR_][selC_] = 0;
            changed = true;
        }
        break;

    default: {
        int d = 0;
        if (key >= '1' && key <= '9')
            d = (int)(key - '0');
        else if (key >= VK_NUMPAD1 && key <= VK_NUMPAD9)
            d = (int)(key - VK_NUMPAD1 + 1);

        if (d && !given_[selR_][selC_]) {
            board_[selR_][selC_] = d;
            changed = true;
        }
    } break;
    }

    if (changed) redraw();
}

SudokuApp& app()
{
    static SudokuApp instance;
    return instance;
}
```

### 7.7 `sudoku_core.h`

```cpp
#pragma once

namespace sudoku {

constexpr int N = 9;

// 检查盘面是否自相矛盾（同行/同列/同宫有重复）
bool validate(const int board[N][N]);

// 用回溯 + MRV 求解；成功返回 true，并把答案写回 board
bool solve(int board[N][N]);

} // namespace sudoku
```

### 7.8 `sudoku_core.cpp`

```cpp
#include "sudoku_core.h"

namespace sudoku {

namespace {

inline int popcount9(int x) {
    int n = 0;
    while (x) { x &= (x - 1); ++n; }
    return n;
}

bool solveRec(int b[N][N], int rm[9], int cm[9], int bm[9], long long& nodes)
{
    if (++nodes > 20000000LL) return false;   // 安全阀

    int bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (b[r][c]) continue;
            int used  = rm[r] | cm[c] | bm[(r / 3) * 3 + c / 3];
            int avail = (~used) & 0x1FF;
            int cnt   = popcount9(avail);
            if (cnt == 0) return false;
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
        }
    }
    if (bestR == -1) return true;

    int bx = (bestR / 3) * 3 + bestC / 3;
    for (int d = 1; d <= 9; ++d) {
        int bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;

        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        if (solveRec(b, rm, cm, bm, nodes)) return true;
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
    }
    return false;
}

} // namespace

bool validate(const int b[N][N])
{
    int rm[9] = {}, cm[9] = {}, bm[9] = {};
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c) {
            int v = b[r][c];
            if (!v) continue;
            int bit = 1 << (v - 1), bx = (r / 3) * 3 + c / 3;
            if ((rm[r] | cm[c] | bm[bx]) & bit) return false;
            rm[r] |= bit; cm[c] |= bit; bm[bx] |= bit;
        }
    return true;
}

bool solve(int b[N][N])
{
    if (!validate(b)) return false;

    int rm[9] = {}, cm[9] = {}, bm[9] = {};
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c) {
            int v = b[r][c];
            if (!v) continue;
            int bit = 1 << (v - 1), bx = (r / 3) * 3 + c / 3;
            rm[r] |= bit; cm[c] |= bit; bm[bx] |= bit;
        }

    long long nodes = 0;
    return solveRec(b, rm, cm, bm, nodes);
}

} // namespace sudoku
```

### 7.9 `renderer.h`

```cpp
#pragma once
#include <windows.h>

namespace renderer {
    void draw(HDC hdc);
}
```

### 7.10 `renderer.cpp`

```cpp
#include "renderer.h"
#include "app.h"

namespace {

void fillRect(HDC hdc, int x0, int y0, int x1, int y1, COLORREF color)
{
    HBRUSH br = CreateSolidBrush(color);
    RECT rc = { x0, y0, x1, y1 };
    FillRect(hdc, &rc, br);
    DeleteObject(br);
}

void drawGrid(HDC hdc)
{
    HPEN penThin  = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HPEN penThick = CreatePen(PS_SOLID, 3, RGB(40, 40, 40));

    for (int i = 0; i <= sudoku::N; ++i) {
        HPEN pen = (i % 3 == 0) ? penThick : penThin;
        HPEN old = (HPEN)SelectObject(hdc, pen);

        int x = BOARD_X + i * CELL_SIZE;
        int y = BOARD_Y + i * CELL_SIZE;

        MoveToEx(hdc, BOARD_X, y, nullptr);
        LineTo  (hdc, BOARD_X + sudoku::N * CELL_SIZE, y);

        MoveToEx(hdc, x, BOARD_Y, nullptr);
        LineTo  (hdc, x, BOARD_Y + sudoku::N * CELL_SIZE);

        SelectObject(hdc, old);
    }

    DeleteObject(penThin);
    DeleteObject(penThick);
}

void drawNumbers(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, app().numberFont());

    for (int r = 0; r < sudoku::N; ++r) {
        for (int c = 0; c < sudoku::N; ++c) {
            int v = app().at(r, c);
            if (!v) continue;

            RECT cr = {
                BOARD_X + c * CELL_SIZE,       BOARD_Y + r * CELL_SIZE,
                BOARD_X + (c + 1) * CELL_SIZE, BOARD_Y + (r + 1) * CELL_SIZE
            };
            wchar_t buf[4];
            wsprintfW(buf, L"%d", v);
            SetTextColor(hdc, app().givenAt(r, c) ? RGB(20, 20, 20)
                                                  : RGB(0, 110, 220));
            DrawTextW(hdc, buf, -1, &cr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
}

void drawHint(HDC hdc)
{
    SelectObject(hdc, app().uiFont());
    SetTextColor(hdc, RGB(80, 80, 80));

    RECT tr = { BOARD_X, 12, BOARD_X + sudoku::N * CELL_SIZE, 48 };
    DrawTextW(hdc,
        L"点击格子选中，按 1-9 输入，Backspace 清除；蓝色数字由程序求解得出",
        -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

} // namespace

void renderer::draw(HDC hdc)
{
    // 窗口背景
    fillRect(hdc, 0, 0, CLIENT_W, CLIENT_H, RGB(240, 242, 245));

    // 棋盘白底
    fillRect(hdc,
             BOARD_X, BOARD_Y,
             BOARD_X + sudoku::N * CELL_SIZE,
             BOARD_Y + sudoku::N * CELL_SIZE,
             RGB(255, 255, 255));

    // 选中格高亮
    int sr = app().selRow(), sc = app().selCol();
    if (sr >= 0 && sc >= 0) {
        fillRect(hdc,
                 BOARD_X + sc * CELL_SIZE,       BOARD_Y + sr * CELL_SIZE,
                 BOARD_X + (sc + 1) * CELL_SIZE, BOARD_Y + (sr + 1) * CELL_SIZE,
                 RGB(186, 218, 255));
    }

    drawGrid(hdc);
    drawNumbers(hdc);
    drawHint(hdc);
}
```

---

> 全文完。如果本文对你有帮助，欢迎点赞、收藏或转发；有任何问题或改进建议，欢迎在评论区交流。
