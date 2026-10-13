'use strict';

/* =====================================================================
 * 数独求解器 · 网页版
 * 核心算法 1:1 移植自 sudoku_core.cpp（MRV 回溯 + 唯一解挖洞）
 * 交互逻辑与文案移植自 engine.cpp / Main.qml（桌面版）
 * ===================================================================== */

const N = 9;
const K_CLUES = [42, 34, 28, 24]; // 简单/中等/困难/专家 的提示数

/* ---------------- 基础工具 ---------------- */

function popcount9(x) {
    let n = 0;
    while (x) { x &= (x - 1); ++n; }
    return n;
}

function emptyBoard() {
    return Array.from({ length: N }, () => new Array(N).fill(0));
}

function cloneBoard(b) {
    return b.map(row => row.slice());
}

function buildMasks(b) {
    const rm = new Array(N).fill(0);
    const cm = new Array(N).fill(0);
    const bm = new Array(N).fill(0);
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            const v = b[r][c];
            if (!v) continue;
            const bit = 1 << (v - 1);
            rm[r] |= bit; cm[c] |= bit; bm[Math.floor(r / 3) * 3 + Math.floor(c / 3)] |= bit;
        }
    return { rm, cm, bm };
}

function shuffled9() {
    const order = [1, 2, 3, 4, 5, 6, 7, 8, 9];
    for (let i = 8; i > 0; --i) {
        const j = Math.floor(Math.random() * (i + 1));
        [order[i], order[j]] = [order[j], order[i]];
    }
    return order;
}

function shuffled81() {
    const order = Array.from({ length: 81 }, (_, i) => i);
    for (let i = 80; i > 0; --i) {
        const j = Math.floor(Math.random() * (i + 1));
        [order[i], order[j]] = [order[j], order[i]];
    }
    return order;
}

/* ---------------- 校验 ---------------- */

function validate(b) {
    const rm = new Array(N).fill(0);
    const cm = new Array(N).fill(0);
    const bm = new Array(N).fill(0);
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            const v = b[r][c];
            if (!v) continue;
            const bit = 1 << (v - 1);
            const bx = Math.floor(r / 3) * 3 + Math.floor(c / 3);
            if ((rm[r] | cm[c] | bm[bx]) & bit) return false;
            rm[r] |= bit; cm[c] |= bit; bm[bx] |= bit;
        }
    return true;
}

/* ---------------- 求解（回溯 + MRV，2000 万节点安全阀） ---------------- */

let solveNodes = 0;

function solveRec(b, rm, cm, bm) {
    if (++solveNodes > 20000000) return false; // 安全阀

    let bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            if (b[r][c]) continue;
            const used = rm[r] | cm[c] | bm[Math.floor(r / 3) * 3 + Math.floor(c / 3)];
            const avail = (~used) & 0x1FF;
            const cnt = popcount9(avail);
            if (cnt === 0) return false;
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
        }
    if (bestR === -1) return true;

    const bx = Math.floor(bestR / 3) * 3 + Math.floor(bestC / 3);
    for (let d = 1; d <= 9; ++d) {
        const bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;
        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        if (solveRec(b, rm, cm, bm)) return true;
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
    }
    return false;
}

function solve(b) {
    if (!validate(b)) return false;
    const { rm, cm, bm } = buildMasks(b);
    solveNodes = 0;
    return solveRec(b, rm, cm, bm);
}

/* ---------------- 生成（随机填满 → 挖洞 → 保证唯一解） ---------------- */

function fillRec(b, rm, cm, bm) {
    let bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            if (b[r][c]) continue;
            const used = rm[r] | cm[c] | bm[Math.floor(r / 3) * 3 + Math.floor(c / 3)];
            const avail = (~used) & 0x1FF;
            const cnt = popcount9(avail);
            if (cnt === 0) return false;
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
        }
    if (bestR === -1) return true;

    const order = shuffled9();
    const bx = Math.floor(bestR / 3) * 3 + Math.floor(bestC / 3);
    for (const d of order) {
        const bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;
        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        if (fillRec(b, rm, cm, bm)) return true;
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
    }
    return false;
}

// 数解数，数到 limit 即停（唯一性检查只需数到 2）
function countRec(b, rm, cm, bm, limit) {
    let bestR = -1, bestC = -1, bestAvail = 0, bestCnt = 10;
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            if (b[r][c]) continue;
            const used = rm[r] | cm[c] | bm[Math.floor(r / 3) * 3 + Math.floor(c / 3)];
            const avail = (~used) & 0x1FF;
            const cnt = popcount9(avail);
            if (cnt === 0) return 0;
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; bestAvail = avail; }
        }
    if (bestR === -1) return 1;

    const bx = Math.floor(bestR / 3) * 3 + Math.floor(bestC / 3);
    let total = 0;
    for (let d = 1; d <= 9; ++d) {
        const bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;
        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        total += countRec(b, rm, cm, bm, limit);
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
        if (total >= limit) return total;
    }
    return total;
}

function countSolutions(src, limit) {
    const b = cloneBoard(src);
    const { rm, cm, bm } = buildMasks(b);
    return countRec(b, rm, cm, bm, limit);
}

function generate(difficulty) {
    const target = 81 - K_CLUES[difficulty];
    const attempts = (difficulty >= 2) ? 256 : 64; // 困难/专家重试更多

    for (let attempt = 0; attempt < attempts; ++attempt) {
        const full = emptyBoard();
        const { rm, cm, bm } = buildMasks(full);
        if (!fillRec(full, rm, cm, bm)) continue;

        const puzzle = cloneBoard(full);
        const order = shuffled81();

        let removed = 0;
        for (let i = 0; i < 81 && removed < target; ++i) {
            const r = Math.floor(order[i] / 9), c = order[i] % 9;
            const save = puzzle[r][c];
            puzzle[r][c] = 0;
            if (countSolutions(puzzle, 2) === 1) ++removed;
            else puzzle[r][c] = save;
        }

        if (removed === target) return puzzle;
    }
    return null;
}

/* =====================================================================
 * 引擎状态（移植自 engine.cpp）
 * ===================================================================== */

const SAMPLE = [
    "530070000",
    "600195000",
    "098000060",
    "800060003",
    "400803001",
    "700020006",
    "060000280",
    "000419005",
    "000080079"
];

const DIFF_NAMES = ["简单", "中等", "困难", "专家"];
const HINT_TEXT = "点击格子选中，按 1-9 输入，⌫ 清除";

const engine = {
    board: emptyBoard(),
    given: emptyBoard(),
    conflicts: emptyBoard(),
    selRow: -1,
    selCol: -1,
    difficulty: 0,
    statusText: "",
    statusError: false,

    setStatus(text, error) {
        if (this.statusText === text && this.statusError === error) return;
        this.statusText = text;
        this.statusError = error;
    },

    hasAnyConflict() {
        for (let r = 0; r < N; ++r)
            for (let c = 0; c < N; ++c)
                if (this.conflicts[r][c]) return true;
        return false;
    },

    recomputeConflicts() {
        const b = this.board, cf = this.conflicts;
        for (let r = 0; r < N; ++r) cf[r].fill(false);
        for (let r = 0; r < N; ++r)
            for (let c = 0; c < N; ++c) {
                const v = b[r][c];
                if (!v) continue;
                const bx = Math.floor(r / 3) * 3, by = Math.floor(c / 3) * 3;
                for (let k = 0; k < N; ++k) {
                    if (k !== c && b[r][k] === v) cf[r][c] = true;              // 同行
                    if (k !== r && b[k][c] === v) cf[r][c] = true;              // 同列
                    const rr = bx + Math.floor(k / 3), cc = by + (k % 3);       // 同宫
                    if ((rr !== r || cc !== c) && b[rr][cc] === v) cf[r][c] = true;
                }
            }
    },

    loadBoard(src, markGiven) {
        this.board = cloneBoard(src);
        this.given = emptyBoard();
        this.conflicts = emptyBoard();
        if (markGiven)
            for (let r = 0; r < N; ++r)
                for (let c = 0; c < N; ++c)
                    this.given[r][c] = (src[r][c] !== 0);
        this.selRow = this.selCol = -1;
    },

    select(r, c) {
        if (r < 0 || r >= N || c < 0 || c >= N) return;
        if (this.selRow === r && this.selCol === c) return;
        this.selRow = r;
        this.selCol = c;
        render();
    },

    moveSel(dr, dc) {
        if (this.selRow < 0) { this.select(0, 0); return; }
        let r = Math.min(Math.max(this.selRow + dr, 0), N - 1);
        let c = Math.min(Math.max(this.selCol + dc, 0), N - 1);
        this.select(r, c);
    },

    inputDigit(digit) {
        if (this.selRow < 0) {
            this.setStatus("请先点击选中一个格子", false);
            render();
            return;
        }
        if (digit < 0 || digit > 9) return;

        const r = this.selRow, c = this.selCol;
        if (this.given[r][c]) {
            this.setStatus("该格是题目给定数字，不能修改", true);
            render();
            return;
        }

        this.board[r][c] = (digit === 0) ? 0 : digit;
        this.recomputeConflicts();

        // 完成检测
        let full = true;
        for (let i = 0; i < N && full; ++i)
            for (let j = 0; j < N; ++j)
                if (!this.board[i][j]) { full = false; break; }

        if (full && validate(this.board)) {
            this.setStatus("恭喜完成！", false);
        } else if (this.hasAnyConflict()) {
            this.setStatus("盘面存在冲突，请检查红色标记", true);
        } else {
            this.setStatus(HINT_TEXT, false);
        }
        render();
    },

    setDifficulty(d) {
        if (d < 0) d = 0;
        if (d > 3) d = 3;
        if (this.difficulty === d) return;
        this.difficulty = d;
        render();
    },

    newGame() {
        this.setStatus("正在生成新题目…", false);
        render();
        // 让状态栏先刷新再同步生成（算法很快，不阻塞感知）
        setTimeout(() => {
            const b = generate(this.difficulty);
            if (!b) {
                this.setStatus("生成失败，请重试", true);
                render();
                return;
            }
            this.loadBoard(b, true);
            this.setStatus("已生成新题目 · " + DIFF_NAMES[this.difficulty], false);
            render();
        }, 30);
    },

    loadSample() {
        const b = emptyBoard();
        for (let r = 0; r < N; ++r)
            for (let c = 0; c < N; ++c)
                b[r][c] = (SAMPLE[r][c] === '0') ? 0 : (SAMPLE[r][c] - '0');
        this.loadBoard(b, true);
        this.setStatus("已载入示例题目", false);
        render();
    },

    clearBoard() {
        this.loadBoard(emptyBoard(), false);
        this.setStatus("已清空盘面", false);
        render();
    },

    solveCurrent() {
        if (this.hasAnyConflict()) {
            this.setStatus("盘面存在冲突，请先修正", true);
            render();
            return;
        }
        const tmp = cloneBoard(this.board);
        if (!solve(tmp)) {
            this.setStatus("该数独无解", true);
            render();
            return;
        }
        this.board = tmp;
        this.recomputeConflicts();
        this.setStatus("求解完成，蓝色数字为程序求出", false);
        render();
    }
};

/* =====================================================================
 * 界面（移植自 Main.qml 的布局与配色）
 * ===================================================================== */

let cells = []; // 81 个格子 DOM，cells[r*9+c]

function cellClasses(r, c, v) {
    const idx = r * 9 + c;
    const isSel = idx === engine.selRow * 9 + engine.selCol && engine.selRow >= 0;
    let isPeer = false;
    if (engine.selRow >= 0 && !isSel) {
        const sr = engine.selRow, sc = engine.selCol;
        isPeer = (r === sr || c === sc ||
            (Math.floor(r / 3) === Math.floor(sr / 3) &&
             Math.floor(c / 3) === Math.floor(sc / 3)));
    }
    const sameVal = v > 0 && engine.selRow >= 0 && !isSel &&
        v === engine.board[engine.selRow][engine.selCol];

    const cls = ["cell"];
    // 网格线
    cls.push(c % 3 === 2 ? (c === 8 ? "" : "br-thick") : (c === 8 ? "" : "br-thin"));
    cls.push(r % 3 === 2 ? (r === 8 ? "" : "bb-thick") : (r === 8 ? "" : "bb-thin"));

    if (engine.conflicts[r][c]) cls.push("conflict");
    else if (isSel) cls.push("sel");
    else if (isPeer || sameVal) cls.push("peer");
    else if (engine.given[r][c]) cls.push("given");

    return cls.filter(Boolean).join(" ");
}

function render() {
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            const v = engine.board[r][c];
            const el = cells[r * 9 + c];
            el.textContent = v > 0 ? String(v) : "";
            el.className = cellClasses(r, c, v);
        }

    // 难度 toggle
    document.querySelectorAll("#diff-group .btn.toggle").forEach(btn => {
        btn.classList.toggle("checked", Number(btn.dataset.diff) === engine.difficulty);
    });

    // 状态栏
    const status = document.getElementById("status");
    status.textContent = engine.statusText;
    status.classList.toggle("error", engine.statusError);
}

function initUI() {
    // 棋盘
    const board = document.getElementById("board");
    for (let r = 0; r < N; ++r)
        for (let c = 0; c < N; ++c) {
            const el = document.createElement("div");
            el.className = "cell";
            el.addEventListener("click", () => { engine.select(r, c); });
            board.appendChild(el);
            cells.push(el);
        }

    // 难度按钮
    const diffGroup = document.getElementById("diff-group");
    DIFF_NAMES.forEach((name, d) => {
        const btn = document.createElement("button");
        btn.className = "btn toggle";
        btn.dataset.diff = String(d);
        btn.textContent = name;
        btn.addEventListener("click", () => engine.setDifficulty(d));
        diffGroup.appendChild(btn);
    });

    // 数字键盘
    const padGroup = document.getElementById("pad-group");
    for (let i = 0; i < 10; ++i) {
        const btn = document.createElement("button");
        btn.className = "btn pad-btn";
        btn.textContent = (i === 9) ? "⌫" : String(i + 1);
        btn.addEventListener("click", () => engine.inputDigit(i === 9 ? 0 : i + 1));
        padGroup.appendChild(btn);
    }

    // 操作按钮
    document.getElementById("btn-new").addEventListener("click", () => engine.newGame());
    document.getElementById("btn-sample").addEventListener("click", () => engine.loadSample());
    document.getElementById("btn-solve").addEventListener("click", () => engine.solveCurrent());
    document.getElementById("btn-clear").addEventListener("click", () => engine.clearBoard());

    // 键盘
    document.addEventListener("keydown", (e) => {
        const k = e.key;
        let handled = true;
        if (k >= "1" && k <= "9") engine.inputDigit(k.charCodeAt(0) - 48);
        else if (k === "0") engine.inputDigit(0);
        else if (k === "Backspace" || k === "Delete") engine.inputDigit(0);
        else if (k === "ArrowLeft") engine.moveSel(0, -1);
        else if (k === "ArrowRight") engine.moveSel(0, 1);
        else if (k === "ArrowUp") engine.moveSel(-1, 0);
        else if (k === "ArrowDown") engine.moveSel(1, 0);
        else handled = false;
        if (handled) e.preventDefault();
    });

    // 启动：与桌面版一致，先载入示例题目
    engine.loadSample();
}

/* ---------------- 浏览器 / Node 双环境 ---------------- */

if (typeof document !== "undefined" && typeof window !== "undefined") {
    document.addEventListener("DOMContentLoaded", initUI);
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = {
        N, K_CLUES, SAMPLE,
        validate, solve, generate, countSolutions
    };
}
