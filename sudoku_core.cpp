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