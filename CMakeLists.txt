cmake_minimum_required(VERSION 3.20)
project(SudokuGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WIN32 关键字 = 自动设置 /SUBSYSTEM:WINDOWS，不用再手动改属性
add_executable(sudoku_gui WIN32
    main.cpp
    main_window.cpp
    app.cpp
    renderer.cpp
    sudoku_core.cpp
)

target_link_libraries(sudoku_gui PRIVATE user32 gdi32)

if (MSVC)
    # 强制 UTF-8，避免中文注释乱码
    target_compile_options(sudoku_gui PRIVATE /utf-8)
endif()