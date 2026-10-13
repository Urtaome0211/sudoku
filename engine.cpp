#include "engine.h"

#include <cstring>

namespace {

const char *kSample[9] = {
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

const char *kDiffName[4] = { "简单", "中等", "困难", "专家" };

const char *kHintText = "点击格子选中，按 1-9 输入，⌫ 清除";

} // namespace

SudokuEngine &SudokuEngine::instance()
{
    static SudokuEngine e;
    return e;
}

SudokuEngine::SudokuEngine(QObject *parent)
    : QObject(parent)
{
    loadSample();
}

void SudokuEngine::setStatus(const QString &text, bool error)
{
    if (statusText_ == text && statusError_ == error) return;
    statusText_ = text;
    statusError_ = error;
    emit statusTextChanged();
}

bool SudokuEngine::hasAnyConflict() const
{
    for (int r = 0; r < sudoku::N; ++r)
        for (int c = 0; c < sudoku::N; ++c)
            if (conflict_[r][c]) return true;
    return false;
}

void SudokuEngine::recomputeConflicts()
{
    std::memset(conflict_, 0, sizeof(conflict_));
    for (int r = 0; r < sudoku::N; ++r) {
        for (int c = 0; c < sudoku::N; ++c) {
            int v = board_[r][c];
            if (!v) continue;

            const int bx = (r / 3) * 3, by = (c / 3) * 3;
            for (int k = 0; k < sudoku::N; ++k) {
                if (k != c && board_[r][k] == v) conflict_[r][c] = true;            // 同行
                if (k != r && board_[k][c] == v) conflict_[r][c] = true;            // 同列
                int rr = bx + k / 3, cc = by + k % 3;                               // 同宫
                if ((rr != r || cc != c) && board_[rr][cc] == v) conflict_[r][c] = true;
            }
        }
    }
}

void SudokuEngine::rebuild()
{
    cells_.clear();
    givens_.clear();
    conflicts_.clear();
    cells_.reserve(sudoku::N * sudoku::N);
    givens_.reserve(sudoku::N * sudoku::N);
    conflicts_.reserve(sudoku::N * sudoku::N);

    for (int r = 0; r < sudoku::N; ++r)
        for (int c = 0; c < sudoku::N; ++c) {
            cells_     << board_[r][c];
            givens_    << given_[r][c];
            conflicts_ << conflict_[r][c];
        }

    emit cellsChanged();
    emit givensChanged();
    emit conflictsChanged();
}

void SudokuEngine::select(int r, int c)
{
    if (r < 0 || r >= sudoku::N || c < 0 || c >= sudoku::N) return;
    if (selRow_ == r && selCol_ == c) return;
    selRow_ = r;
    selCol_ = c;
    selIndex_ = r * sudoku::N + c;
    emit selChanged();
}

void SudokuEngine::moveSel(int dr, int dc)
{
    if (selRow_ < 0) { select(0, 0); return; }
    int r = selRow_ + dr;
    int c = selCol_ + dc;
    if (r < 0) r = 0;
    if (r >= sudoku::N) r = sudoku::N - 1;
    if (c < 0) c = 0;
    if (c >= sudoku::N) c = sudoku::N - 1;
    select(r, c);
}

void SudokuEngine::inputDigit(int digit)
{
    if (selRow_ < 0) {
        setStatus(QString::fromUtf8("请先点击选中一个格子"), false);
        return;
    }
    if (digit < 0 || digit > 9) return;

    if (given_[selRow_][selCol_]) {
        setStatus(QString::fromUtf8("该格是题目给定数字，不能修改"), true);
        return;
    }

    board_[selRow_][selCol_] = (digit == 0) ? 0 : digit;
    recomputeConflicts();
    rebuild();

    // 完成检测
    bool full = true;
    for (int r = 0; r < sudoku::N && full; ++r)
        for (int c = 0; c < sudoku::N; ++c)
            if (!board_[r][c]) { full = false; break; }

    if (full && sudoku::validate(board_)) {
        setStatus(QString::fromUtf8("恭喜完成！"), false);
        return;
    }
    if (hasAnyConflict())
        setStatus(QString::fromUtf8("盘面存在冲突，请检查红色标记"), true);
    else
        setStatus(QString::fromUtf8(kHintText), false);
}

void SudokuEngine::setDifficulty(int d)
{
    if (d < 0) d = 0;
    if (d > 3) d = 3;
    if (difficulty_ == d) return;
    difficulty_ = d;
    emit difficultyChanged();
}

void SudokuEngine::newGame()
{
    int b[sudoku::N][sudoku::N] = {};
    if (!sudoku::generate(b, static_cast<sudoku::Difficulty>(difficulty_))) {
        setStatus(QString::fromUtf8("生成失败，请重试"), true);
        return;
    }

    std::memcpy(board_, b, sizeof(board_));
    std::memset(given_, 0, sizeof(given_));
    for (int r = 0; r < sudoku::N; ++r)
        for (int c = 0; c < sudoku::N; ++c)
            given_[r][c] = (b[r][c] != 0);

    selRow_ = selCol_ = selIndex_ = -1;
    std::memset(conflict_, 0, sizeof(conflict_));
    rebuild();
    emit selChanged();
    setStatus(QString::fromUtf8("已生成新题目 · %1").arg(QString::fromUtf8(kDiffName[difficulty_])), false);
}

void SudokuEngine::loadSample()
{
    for (int r = 0; r < sudoku::N; ++r)
        for (int c = 0; c < sudoku::N; ++c) {
            char ch = kSample[r][c];
            board_[r][c] = (ch == '0') ? 0 : (ch - '0');
            given_[r][c] = (ch != '0');
        }

    selRow_ = selCol_ = selIndex_ = -1;
    std::memset(conflict_, 0, sizeof(conflict_));
    rebuild();
    emit selChanged();
    setStatus(QString::fromUtf8("已载入示例题目"), false);
}

void SudokuEngine::clearBoard()
{
    std::memset(board_, 0, sizeof(board_));
    std::memset(given_, 0, sizeof(given_));
    std::memset(conflict_, 0, sizeof(conflict_));
    selRow_ = selCol_ = selIndex_ = -1;
    rebuild();
    emit selChanged();
    setStatus(QString::fromUtf8("已清空盘面"), false);
}

void SudokuEngine::solveCurrent()
{
    if (hasAnyConflict()) {
        setStatus(QString::fromUtf8("盘面存在冲突，请先修正"), true);
        return;
    }

    int tmp[sudoku::N][sudoku::N];
    std::memcpy(tmp, board_, sizeof(tmp));
    if (!sudoku::solve(tmp)) {
        setStatus(QString::fromUtf8("该数独无解"), true);
        return;
    }

    std::memcpy(board_, tmp, sizeof(board_));
    recomputeConflicts();
    rebuild();
    setStatus(QString::fromUtf8("求解完成，蓝色数字为程序求出"), false);
}
