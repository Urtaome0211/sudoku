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