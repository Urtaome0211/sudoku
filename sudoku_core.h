#pragma once

namespace sudoku {

constexpr int N = 9;

// 检查盘面是否自相矛盾（同行/同列/同宫有重复）
bool validate(const int board[N][N]);

// 用回溯 + MRV 求解；成功返回 true，并把答案写回 board
bool solve(int board[N][N]);

} // namespace sudoku