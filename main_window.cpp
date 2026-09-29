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