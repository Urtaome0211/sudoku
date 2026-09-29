#pragma once
#include <windows.h>

namespace main_window {

// 注册窗口类
void registerClass(HINSTANCE hInstance);

// 创建并显示主窗口，失败返回 nullptr
HWND create(HINSTANCE hInstance, int nCmdShow);

} // namespace main_window