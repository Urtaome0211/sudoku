#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QDir>
#include <QtQml/qqml.h>

#include "engine.h"

// 离屏 / minimal 平台走的是 Qt 基础字体引擎（不查询系统字体服务），而 Qt 已不再内置字体，
// 若不指定字体目录，所有文字都会渲染成空心方框。这里在创建 QGuiApplication 之前自动兜底。
static void ensureHeadlessFontDir()
{
    const QByteArray platform = qgetenv("QT_QPA_PLATFORM");
    if (platform != "offscreen" && platform != "minimal")
        return;
    if (!qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR"))
        return;

    const QStringList candidates = {
#ifdef Q_OS_WIN
        QStringLiteral("C:/Windows/Fonts"),
#else
        QStringLiteral("/usr/share/fonts"),
        QStringLiteral("/usr/local/share/fonts"),
        QStringLiteral("/System/Library/Fonts"),
#endif
    };
    for (const QString &dir : candidates) {
        if (QDir(dir).exists()) {
            qputenv("QT_QPA_FONTDIR", dir.toLocal8Bit());
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    ensureHeadlessFontDir();

    QGuiApplication app(argc, argv);

    // Fluent WinUI3 扁平风格（qtquickcontrols2.conf 双保险）
    QQuickStyle::setStyle("FluentWinUI3");

    // 可选参数：--screenshot <文件> [--scenario <sample|expert|solved>]，用于离屏自检
    QString shotPath;
    QString scenario = QStringLiteral("sample");
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QLatin1String("--screenshot") && i + 1 < args.size())
            shotPath = args[++i];
        else if (args[i] == QLatin1String("--scenario") && i + 1 < args.size())
            scenario = args[++i];
    }

    qmlRegisterSingletonInstance("Sudoku", 1, 0, "Engine", &SudokuEngine::instance());

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Sudoku", "Main");
    if (engine.rootObjects().isEmpty()) return 1;

    if (!shotPath.isEmpty()) {
        if (scenario == QLatin1String("expert")) {
            SudokuEngine::instance().setDifficulty(3);
            SudokuEngine::instance().newGame();
            SudokuEngine::instance().select(4, 4);
        } else if (scenario == QLatin1String("solved")) {
            SudokuEngine::instance().solveCurrent();
        }

        QTimer::singleShot(600, [&engine, shotPath]() {
            auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
            if (win && win->grabWindow().save(shotPath))
                QCoreApplication::exit(0);
            else
                QCoreApplication::exit(2);
        });
    }

    return app.exec();
}
