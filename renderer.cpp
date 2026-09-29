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