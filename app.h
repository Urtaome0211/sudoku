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