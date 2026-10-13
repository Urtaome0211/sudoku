# 数独求解器重构记：从纯 Win32 到 Qt Quick / Fluent 与零依赖网页版

> 在上一篇《从零写一个 Windows 原生数独求解器：纯 Win32 + 位掩码回溯》里，我们写了一个约 500 行的 Win32 + GDI 数独小程序：点击选格、键盘输入、一键求解。这一次把它彻底重做：**保留核心算法**，用 **Qt Quick + QML（Fluent WinUI3 风格）** 重写桌面版，并顺手产出一个**零依赖网页版**。新版加入**随机题目生成（唯一解保证）**、**四档难度**、**冲突红色高亮**，界面全面扁平化。本文按重构的真实过程展开：架构如何演进、出题算法怎么写、两个 UI 如何共享同一套逻辑，以及踩过的坑。

---

## 目录

1. [为什么要重构](#1-为什么要重构)
2. [新架构：算法内核与两个外壳](#2-新架构算法内核与两个外壳)
3. [随机题目生成：挖洞法 + 唯一解校验](#3-随机题目生成挖洞法--唯一解校验)
4. [四档难度：少一个提示数，多一分困难](#4-四档难度少一个提示数多一分困难)
5. [Qt Quick 界面：声明式 UI 与扁平化](#5-qt-quick-界面声明式-ui-与扁平化)
6. [网页版：同一套算法的 JS 移植](#6-网页版同一套算法的-js-移植)
7. [工程化与测试](#7-工程化与测试)
8. [踩坑记录](#8-踩坑记录)
9. [总结](#9-总结)

---

## 1. 为什么要重构

旧版有三个明显的短板：

1. **只能求解，不能出题** —— 题目得自己去找；
2. **界面过时** —— Win32 灰框按钮 + 系统默认外观，谈不上设计；
3. **无冲突提示** —— 输入矛盾数字时毫无反馈，直到点"求解"才被告知盘面有冲突。

新版目标：随机新题（四档难度）、冲突实时高亮、现代化的扁平界面。选型结论：

| 部分 | 方案 | 理由 |
|------|------|------|
| 桌面版 | Qt Quick / QML + FluentWinUI3 风格 | 声明式 UI 与"扁平化重绘"天然契合；原算法层纯 C++，直接复用 |
| 网页版 | 纯 HTML/CSS/JS，零依赖 | 无构建、无服务器，双击即玩；算法用 JS 重写一份 |
| 界面风格 | 与桌面版同款扁平配色体系 | 两个平台视觉一致，配色用变量集中管理 |

---

## 2. 新架构：算法内核与两个外壳

旧版是命令式四层结构（`main_window → app → sudoku_core`，`renderer` 每帧逐层绘制）；新版的核心思想是 **算法层与 UI 层彻底解耦** —— `sudoku_core` 只跟 `int board[9][9]` 打交道，不碰任何窗口、QML 或 DOM 类型，这正是它能连续服务三个 UI 的原因：

```
旧（Win32，命令式）：
main_window ──► app ──► sudoku_core
    └──────────► renderer ──► app

新（Qt Quick，声明式）：
main.cpp ──► SudokuEngine（QObject 单例）──► sudoku_core（原样保留）
Main.qml ◄── Q_PROPERTY / Q_INVOKABLE ──────┘

网页版（零依赖）：
index.html / style.css ──► app.js: engine 对象 ──► generate / solve / validate
test.js（Node 直接 require 同一份 app.js）──────────┘
```

桌面版的引擎接口（`engine.h`）：状态用 `Q_PROPERTY` 暴露给 QML 自动绑定，操作用 `Q_INVOKABLE` 暴露给 QML 调用：

```cpp
Q_PROPERTY(QVariantList cells     READ cells     NOTIFY cellsChanged)
Q_PROPERTY(QVariantList givens    READ givens    NOTIFY givensChanged)
Q_PROPERTY(QVariantList conflicts READ conflicts NOTIFY conflictsChanged)
Q_PROPERTY(int selRow   READ selRow   NOTIFY selChanged)
Q_PROPERTY(int selCol   READ selCol   NOTIFY selChanged)
Q_PROPERTY(int difficulty READ difficulty NOTIFY difficultyChanged)
Q_PROPERTY(QString statusText  READ statusText  NOTIFY statusTextChanged)
Q_PROPERTY(bool   statusError READ statusError NOTIFY statusTextChanged)

Q_INVOKABLE void select(int r, int c);
Q_INVOKABLE void moveSel(int dr, int dc);
Q_INVOKABLE void inputDigit(int digit);      // 1-9 输入，0 清除
Q_INVOKABLE void setDifficulty(int d);       // 0 简单 ~ 3 专家
Q_INVOKABLE void newGame();
Q_INVOKABLE void loadSample();
Q_INVOKABLE void clearBoard();
Q_INVOKABLE void solveCurrent();
```

UI 层不保存任何状态：`Main.qml` 只负责把 Engine 的属性渲染出来。与旧版 renderer 每帧命令式绘制不同，声明式 UI 里"状态"是唯一真相，"重绘"由框架自动完成。

---

## 3. 随机题目生成：挖洞法 + 唯一解校验

生成算法采用经典的 **挖洞法（dig-hole）**，三步走：

1. **随机填满**：用带回溯的 MRV 搜索随机生成一个合法完整盘（候选数字按随机顺序尝试）；
2. **随机挖洞**：把 81 个位置随机排序，逐个尝试挖掉；
3. **唯一解校验**：每挖一格就数一次剩余盘面的解数，`== 1` 才保留，否则填回。

第一步的关键是随机化 —— 与求解器几乎相同的递归，只是候选顺序打乱：

```cpp
// 随机填满一个合法盘面（候选数字按随机顺序尝试）
bool fillRec(int b[N][N], int rm[9], int cm[9], int bm[9])
{
    // ... 与 solveRec 相同：MRV 选出候选最少的空格 ...
    int order[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::shuffle(order, order + 9, rng());   // 关键：打乱尝试顺序

    int bx = (bestR / 3) * 3 + bestC / 3;
    for (int i = 0; i < 9; ++i) {
        int d = order[i];
        int bit = 1 << (d - 1);
        if (!(bestAvail & bit)) continue;

        b[bestR][bestC] = d; rm[bestR] |= bit; cm[bestC] |= bit; bm[bx] |= bit;
        if (fillRec(b, rm, cm, bm)) return true;
        b[bestR][bestC] = 0; rm[bestR] &= ~bit; cm[bestC] &= ~bit; bm[bx] &= ~bit;
    }
    return false;
}
```

第二步的"数解数"与求解器几乎相同，只是把"找到解就返回"改成"计数，数到 limit 就停"：

```cpp
// 统计解的数量，最多数到 limit 即停（唯一性检查只需要数到 2）
int countRec(int b[N][N], int rm[9], int cm[9], int bm[9], int limit)
{
    // ... 同样 MRV 选空格 ...
    if (bestR == -1) return 1;                 // 没有空格 = 找到一个解

    int total = 0;
    for (int d = 1; d <= 9; ++d) {
        // ... 尝试每个候选数字 ...
        total += countRec(b, rm, cm, bm, limit);
        if (total >= limit) return total;      // 数到 2 立刻剪枝
    }
    return total;
}
```

生成主流程把两步串起来：

```cpp
bool generate(int board[N][N], Difficulty difficulty)
{
    const int target   = 81 - kClues[static_cast<int>(difficulty)];
    const int attempts = (difficulty >= Difficulty::Hard) ? 256 : 64;

    for (int attempt = 0; attempt < attempts; ++attempt) {
        int full[N][N] = {};
        // 1. 随机填满一个完整盘
        if (!fillRec(full, rm, cm, bm)) continue;

        // 2+3. 随机顺序挖空，每挖一格检查唯一解，不唯一则还原
        int order[81];
        for (int i = 0; i < 81; ++i) order[i] = i;
        std::shuffle(order, order + 81, rng());

        int removed = 0;
        for (int i = 0; i < 81 && removed < target; ++i) {
            int r = order[i] / 9, c = order[i] % 9;
            int save = puzzle[r][c];
            puzzle[r][c] = 0;
            if (countSolutions(puzzle, 2) == 1) ++removed;
            else puzzle[r][c] = save;          // 破坏唯一解 → 填回
        }

        if (removed == target) return true;    // 挖满目标数即成功
    }
    return false;
}
```

两个值得注意的细节：

- **`limit = 2` 剪枝**：唯一性检查不需要数清全部解，数到 2 立刻返回。这是生成速度的关键；
- **挖不满就整盘重来**：有时挖到中途，剩下任何一格都不能再挖（一挖就出现多解），达不到目标提示数。此时放弃整盘重新生成 —— 简单/中等最多重试 64 次，困难/专家 256 次。实测单盘生成仅 **2~6 ms**，点击"新游戏"毫无卡顿。

---

## 4. 四档难度：少一个提示数，多一分困难

难度直接由提示数（clues）定义：提示越少，玩家需要自行推理的链条越长：

| 难度 | 提示数 | 挖空格数 | 生成重试上限 |
|------|-------|---------|------------|
| 简单 | 42 | 39 | 64 |
| 中等 | 34 | 47 | 64 |
| 困难 | 28 | 53 | 256 |
| 专家 | 24 | 57 | 256 |

```cpp
const int kClues[4] = { 42, 34, 28, 24 };
```

为什么困难/专家重试更多：提示数越少，"任意再挖一格都不破坏唯一解"的概率越低，一次性挖满 57 格的成功率显著下降，所以给更多重试机会。挖洞法按提示数分难度是最直观的方案，它不严格等价于"解题技巧难度"，但工程上足够好 —— 24 提示数的专家题对普通玩家已经相当有挑战。

---

## 5. Qt Quick 界面：声明式 UI 与扁平化

对比一下旧版的绘制代码和新的 QML。旧版 `renderer.cpp` 手写 `FillRect` / `CreatePen` / `DrawTextW` 逐层绘制；新版 81 个格子的背景色直接用一行表达式声明：

```qml
Repeater {
    model: Engine.cells
    delegate: Rectangle {
        required property int index
        required property var modelData
        readonly property int r: Math.floor(index / 9)
        readonly property int c: index % 9
        readonly property bool isSel: index === Engine.selIndex
        readonly property bool isPeer: Engine.selIndex >= 0 && !isSel &&
            (r === Engine.selRow || c === Engine.selCol ||
             (Math.floor(r / 3) === Math.floor(Engine.selRow / 3) &&
              Math.floor(c / 3) === Math.floor(Engine.selCol / 3)))
        readonly property bool sameVal: modelData > 0 && Engine.selIndex >= 0 &&
            !isSel && modelData === Engine.cells[Engine.selIndex]

        x: c * board.cellSize
        y: r * board.cellSize
        width: board.cellSize
        height: board.cellSize
        // 状态优先级：冲突 > 选中 > 同行列宫/同数 > 悬停 > 空白
        color: Engine.conflicts[index] ? "#FBEBEC"
             : isSel ? "#DCEBFC"
             : (isPeer || sameVal) ? "#EFF5FB"
             : hover.hovered ? "#F5F8FB"
             : "#FFFFFF"

        Text {
            anchors.centerIn: parent
            text: modelData > 0 ? modelData : ""
            font.pixelSize: 24
            color: Engine.conflicts[index] ? "#D64545"     // 冲突红
                 : Engine.givens[index] ? "#20242A"        // 给定黑
                 : "#0F6CBD"                               // 用户/求解蓝
        }

        HoverHandler { id: hover }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: Engine.select(r, c)
        }
    }
}
```

没有任何重绘代码 —— Engine 的信号变了，QML 自动更新对应格子。选中联动（同行/列/宫、同数字）也只是三个 `readonly property` 布尔表达式。冲突检测在 C++ 侧完成（行/列/宫重复双向标红），QML 只需按 `conflicts[index]` 上色。这就是声明式 UI 的核心收益。

扁平按钮用内联组件定义一次，全界面复用（`primary` 主按钮 / `toggle` 难度切换两种形态）：

```qml
component FlatButton: Button {
    id: control
    property bool primary: false
    property bool toggle: false
    checkable: toggle

    contentItem: Text {
        text: control.text
        color: (control.primary || control.checked) ? "#FFFFFF"
             : control.down ? "#0F6CBD"
             : "#3A4250"
    }

    background: Rectangle {
        radius: 6
        color: control.primary ? (control.down ? "#0B5AA7" : control.hovered ? "#0E68BA" : "#0F6CBD")
             : control.checked ? "#0F6CBD"
             : control.down ? "#E8EDF3"
             : control.hovered ? "#F1F4F8"
             : "#FFFFFF"
        border.width: (control.primary || control.checked) ? 0 : 1
        border.color: "#D3D9E0"
    }
}
```

全局风格只需两行：入口 `QQuickStyle::setStyle("FluentWinUI3")` + 资源里的 `qtquickcontrols2.conf` 指定 `Style=FluentWinUI3`，系统控件也统一成 Fluent 观感。完整配色体系（桌面版与网页版共用同一套）：

| 用途 | 色值 |
|------|------|
| 页面背景 | `#F2F4F7` |
| 卡片白 / 边框 | `#FFFFFF` / `#E4E8EE` |
| 网格细线 / 粗线 | `#D5DAE0` / `#9AA3AD` |
| 选中格 / 同行列宫格 | `#DCEBFC` / `#EFF5FB` |
| 冲突底 / 冲突字 | `#FBEBEC` / `#D64545` |
| 题目给定 / 用户求解 | `#20242A` / `#0F6CBD` |
| 主按钮（hover / 按下） | `#0F6CBD`（`#0E68BA` / `#0B5AA7`） |
| 次按钮（hover / 按下） | `#FFFFFF`（`#F1F4F8` / `#E8EDF3`） |

键盘交互由根节点 `Keys.onPressed` 统一处理：`1-9` 与小键盘输入、`0`/`Backspace`/`Delete` 清除、方向键移动选中格，全部转发给 Engine。

---

## 6. 网页版：同一套算法的 JS 移植

网页版零依赖：双击 `index.html` 即玩，无需构建、无需服务器。四个文件：

- `index.html` —— 页面骨架（标题 / 棋盘卡片 / 难度 / 操作 / 数字键盘 / 状态栏）；
- `style.css` —— 与桌面版同款配色，CSS 变量集中定义；
- `app.js` —— 算法 + 交互，一个 `engine` 对象镜像 C++ 引擎的全部行为与文案；
- `test.js` —— Node 自检脚本。

JS 移植几乎逐行对应 C++：

```js
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
```

位掩码逻辑在 JS 里反而更省心：`~used & 0x1FF` 天然就是 32 位整数运算，不用考虑 C++ 的符号扩展；整数除法换成 `Math.floor` 即可。

网页版渲染同样是状态驱动，与 QML 逐条镜像 —— 连"状态优先级"和"网格线画法"都一一对应：

```js
function cellClasses(r, c, v) {
    const idx = r * 9 + c;
    const isSel = idx === engine.selRow * 9 + engine.selCol && engine.selRow >= 0;
    // ... isPeer / sameVal 判断与 QML 完全一致 ...

    const cls = ["cell"];
    // 网格线：每格画右边框/下边框，3 的倍数处加粗，最后一列/行不画
    cls.push(c % 3 === 2 ? (c === 8 ? "" : "br-thick") : (c === 8 ? "" : "br-thin"));
    cls.push(r % 3 === 2 ? (r === 8 ? "" : "bb-thick") : (r === 8 ? "" : "bb-thin"));

    if (engine.conflicts[r][c]) cls.push("conflict");
    else if (isSel) cls.push("sel");
    else if (isPeer || sameVal) cls.push("peer");
    else if (engine.given[r][c]) cls.push("given");

    return cls.filter(Boolean).join(" ");
}
```

棋盘不用 `<canvas>`：每格用 `br/bb-thin` 与 `br/bb-thick` 类画右边框/下边框（1px 细线，3 的倍数处 2px 粗线），容器外圈再加 2px 粗边框，纯 CSS 拼出完整棋盘。

两个 JS 特有的工程技巧：

1. **生成不阻塞 UI**：`newGame` 先用 `setTimeout(30ms)` 渲染"正在生成新题目…"，生成完成后（通常 2~6 ms）再刷新盘面，保证点击有即时反馈；
2. **同一份代码跑浏览器与 Node**：算法函数不碰 DOM，文件末尾按环境导出：

```js
if (typeof module !== "undefined" && module.exports) {
    module.exports = { validate, solve, generate, countSolutions, /* ... */ };
}
```

于是 `test.js` 可以直接 `require` 同一份 `app.js` 做纯逻辑自检，浏览器行为与测试行为永远不会分叉。

---

## 7. 工程化与测试

桌面版 CMake 的关键三段：

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Qml)

qt_add_qml_module(sudoku_gui
    URI Sudoku
    VERSION 1.0
    QML_FILES Main.qml
)

# 控件风格配置文件必须位于资源根目录（:/qtquickcontrols2.conf）
qt_add_resources(sudoku_gui "conf"
    PREFIX "/"
    FILES qtquickcontrols2.conf
)
```

三个测试层次，覆盖"算法 → 界面 → 交互"：

1. **桌面版离屏自检**：`main.cpp` 内置 `--screenshot` 参数，用 offscreen 平台渲染三种场景（示例 / 专家新题 / 求解后）并保存 PNG —— 构建后一条命令即可无窗口回归验证 UI：

   ```powershell
   set QT_QPA_PLATFORM=offscreen
   build-qt\sudoku_gui.exe --screenshot shot.png --scenario expert
   ```

2. **网页版 Node 自检**：`node test.js` —— 示例题目可解、冲突盘面拒绝求解、四档难度 ×3 盘提示数精确（= 42/34/28/24）、每盘唯一解、解合法；
3. **网页版无头浏览器冒烟**：用 Edge `--headless` 截图对比配色、`--dump-dom` 检查 81 格渲染，并模拟点击/输入跑完整交互链路。

---

## 8. 踩坑记录

- **`QQuickStyle` 头文件的位置**：Qt6 里它属于 QtQuickControls2 模块，直接 `#include <QQuickStyle>` 报 `C1083: 无法打开包括文件`；需写 `<QtQuickControls2/QQuickStyle>` 并链接 `Qt6::QuickControls2`；
- **windeployqt 必须带 `--qmldir .`**：不带时 QML 模块与 FluentWinUI3 样式不会被部署（部署目录里 `qml` 文件夹是空的），双击 exe 报 `0xC0000139`（找不到 DLL）；
- **offscreen 插件默认不部署**：windeployqt 只带 `qwindows.dll`；离屏自检用的 `qoffscreen.dll` 需手动从 Qt 安装目录拷贝到 `platforms\`；
- **Edge `--dump-dom` 的输出捕获**：PowerShell 里 `2>$null` 拿不到 stdout，需用 `cmd /c` 重定向到文件再读；
- **唯一解校验不能省**：只在最终盘面数一次解是不够的 —— 必须每挖一格校验一次，否则先挖的格可能提前锁死唯一性，后面再也挖不动，达不到目标提示数。

---

## 9. 总结

这次重构最大的收获是验证了**算法层与 UI 层解耦**的红利：同一个 `sudoku_core`，先服务 Win32，再服务 Qt Quick，又用 JS 重写后服务浏览器 —— 三次 UI 更替，算法一次写成、三处复用。声明式 UI（QML / DOM）用"状态 → 视图"模型取代了命令式逐层绘制，冲突高亮、选中联动这类交互几乎零额外成本；统一的扁平配色体系让两个平台观感完全一致。

对老版本感兴趣的读者可回看上一篇《从零写一个 Windows 原生数独求解器：纯 Win32 + 位掩码回溯》。新版源码：桌面版位于 `qt\` 子目录（Qt 6 + CMake），网页版位于 `web\` 子目录，双击 `web\index.html` 即玩。

---

> 全文完。如果本文对你有帮助，欢迎点赞、收藏或转发；有任何问题或改进建议，欢迎在评论区交流。
