#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqml.h>

#include "engine.h"

int main(int argc, char *argv[])
{
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
