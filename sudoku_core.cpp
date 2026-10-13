#include "sudoku_core.h"

#include <algorithm>
#include <cstring>
#include <random>

namespace sudoku {

namespace {

inline int popcount9(int x)
{
    int n = 0;
    while (x) { x &= (x - 1); ++n; }
    return n;
}

// 每个难度期望保留的提示数
constexpr int kClues[4] = { 42, 34, 28, 24 };

std::mt19937 &rng()
{
    static std::mt19937 g(std::random_device{}());
    return g;
}

bool solveRec(int b[N][N], int rm[9], int cm[9], int bm[9], long long &nodes)
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

// 随机填满一个合法盘面（候选数字按随机顺序尝试）
bool fillRec(int b[N][N], int rm[9], int cm[9], int bm[9])
{
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

    int order[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::shuffle(order, order + 9, rng());

    int bx = (bestR / 3) * 3 + bestC / 3;
    for (int i = 0; i < 9; ++i) {
        int d   = order[i];
        int bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;

        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        if (fillRec(b, rm, cm, bm)) return true;
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
    }
    return false;
}

// 统计解的数量，最多数到 limit 即停（唯一性检查只需要数到 2）
int countRec(int b[N][N], int rm[9], int cm[9], int bm[9], int limit)
{
    int bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (b[r][c]) continue;
            int used  = rm[r] | cm[c] | bm[(r / 3) * 3 + c / 3];
            int avail = (~used) & 0x1FF;
            int cnt   = popcount9(avail);
            if (cnt == 0) return 0;
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
        }
    }
    if (bestR == -1) return 1;

    int bx = (bestR / 3) * 3 + bestC / 3;
    int total = 0;
    for (int d = 1; d <= 9; ++d) {
        int bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;

        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        total += countRec(b, rm, cm, bm, limit);
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
        if (total >= limit) return total;
    }
    return total;
}

int countSolutions(const int src[N][N], int limit)
{
    int b[N][N];
    std::memcpy(b, src, sizeof(b));

    int rm[9] = {}, cm[9] = {}, bm[9] = {};
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c) {
            int v = b[r][c];
            if (!v) continue;
            int bit = 1 << (v - 1), bx = (r / 3) * 3 + c / 3;
            rm[r] |= bit; cm[c] |= bit; bm[bx] |= bit;
        }
    return countRec(b, rm, cm, bm, limit);
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

bool generate(int board[N][N], Difficulty difficulty)
{
    const int target  = 81 - kClues[static_cast<int>(difficulty)];
    const int attempts = (difficulty >= Difficulty::Hard) ? 256 : 64;

    for (int attempt = 0; attempt < attempts; ++attempt) {
        int full[N][N] = {};
        int rm[9] = {}, cm[9] = {}, bm[9] = {};
        if (!fillRec(full, rm, cm, bm)) continue;

        int puzzle[N][N];
        std::memcpy(puzzle, full, sizeof(puzzle));

        // 随机顺序挖空，每挖一格检查唯一解，不唯一则还原
        int order[81];
        for (int i = 0; i < 81; ++i) order[i] = i;
        std::shuffle(order, order + 81, rng());

        int removed = 0;
        for (int i = 0; i < 81 && removed < target; ++i) {
            int r = order[i] / 9, c = order[i] % 9;
            int save = puzzle[r][c];
            puzzle[r][c] = 0;
            if (countSolutions(puzzle, 2) == 1) ++removed;
            else puzzle[r][c] = save;
        }

        if (removed == target) {
            std::memcpy(board, puzzle, sizeof(puzzle));
            return true;
        }
    }
    return false;
}

} // namespace sudoku
