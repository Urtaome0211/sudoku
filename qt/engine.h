#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "sudoku_core.h"

// QML 后端单例：持有盘面状态，提供选择、输入、求解、生成等操作
class SudokuEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList cells     READ cells     NOTIFY cellsChanged)
    Q_PROPERTY(QVariantList givens    READ givens    NOTIFY givensChanged)
    Q_PROPERTY(QVariantList conflicts READ conflicts NOTIFY conflictsChanged)
    Q_PROPERTY(int selRow    READ selRow    NOTIFY selChanged)
    Q_PROPERTY(int selCol    READ selCol    NOTIFY selChanged)
    Q_PROPERTY(int selIndex  READ selIndex  NOTIFY selChanged)
    Q_PROPERTY(int difficulty READ difficulty NOTIFY difficultyChanged)
    Q_PROPERTY(QString statusText  READ statusText  NOTIFY statusTextChanged)
    Q_PROPERTY(bool   statusError READ statusError NOTIFY statusTextChanged)

public:
    static SudokuEngine &instance();

    QVariantList cells() const     { return cells_; }
    QVariantList givens() const    { return givens_; }
    QVariantList conflicts() const { return conflicts_; }
    int  selRow() const   { return selRow_; }
    int  selCol() const   { return selCol_; }
    int  selIndex() const { return selIndex_; }
    int  difficulty() const { return difficulty_; }
    QString statusText() const  { return statusText_; }
    bool   statusError() const { return statusError_; }

    Q_INVOKABLE void select(int r, int c);
    Q_INVOKABLE void moveSel(int dr, int dc);
    Q_INVOKABLE void inputDigit(int digit);      // 1-9 输入，0 清除
    Q_INVOKABLE void setDifficulty(int d);       // 0 简单 ~ 3 专家
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void loadSample();
    Q_INVOKABLE void clearBoard();
    Q_INVOKABLE void solveCurrent();

signals:
    void cellsChanged();
    void givensChanged();
    void conflictsChanged();
    void selChanged();
    void difficultyChanged();
    void statusTextChanged();

private:
    explicit SudokuEngine(QObject *parent = nullptr);

    bool hasAnyConflict() const;
    void recomputeConflicts();
    void rebuild();                              // 同步 QVariantList 并发出信号
    void setStatus(const QString &text, bool error);

    int  board_[sudoku::N][sudoku::N] = {};
    bool given_[sudoku::N][sudoku::N] = {};
    bool conflict_[sudoku::N][sudoku::N] = {};
    int  selRow_ = -1;
    int  selCol_ = -1;
    int  selIndex_ = -1;
    int  difficulty_ = 0;
    QString statusText_;
    bool   statusError_ = false;
    QVariantList cells_, givens_, conflicts_;
};
