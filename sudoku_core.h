#pragma once

namespace sudoku {

constexpr int N = 9;

// 难度等级：越高提示数越少
enum class Difficulty { Easy = 0, Medium = 1, Hard = 2, Expert = 3 };

// 检查盘面是否自相矛盾（同行/同列/同宫有重复）
bool validate(const int board[N][N]);

// 用回溯 + MRV 求解；成功返回 true，并把答案写回 board
bool solve(int board[N][N]);

// 生成随机题目（保证唯一解）；按难度挖空，成功返回 true，并把题目写回 board
bool generate(int board[N][N], Difficulty difficulty);

} // namespace sudoku
