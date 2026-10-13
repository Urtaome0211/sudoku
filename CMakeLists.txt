cmake_minimum_required(VERSION 3.20)
project(SudokuGui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Qml)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(sudoku_gui
    main.cpp
    engine.cpp
    engine.h
    sudoku_core.cpp
    sudoku_core.h
)

qt_add_qml_module(sudoku_gui
    URI Sudoku
    VERSION 1.0
    QML_FILES
        Main.qml
)

# 控件风格配置文件必须位于资源根目录（:/qtquickcontrols2.conf）
qt_add_resources(sudoku_gui "conf"
    PREFIX "/"
    FILES qtquickcontrols2.conf
)

target_link_libraries(sudoku_gui PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Qml)

if (MSVC)
    # 强制 UTF-8，保证中文字符串与注释不乱码
    target_compile_options(sudoku_gui PRIVATE /utf-8)
endif()
