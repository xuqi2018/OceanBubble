cmake_minimum_required(VERSION 3.16)

project(OceanBubble VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 需要为 Q_OBJECT 类生成 moc 代码
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(OceanBubble WIN32
    main.cpp
    Theme.h
    core/GameModel.h
    core/GameModel.cpp
    core/HexGrid.h
    core/HexGrid.cpp
    ui/BoardWidget.h
    ui/BoardWidget.cpp
    ui/BubblePainter.h
    ui/BubblePainter.cpp
    ui/GameWindow.h
    ui/GameWindow.cpp
    ui/PreviewPanel.h
    ui/PreviewPanel.cpp
    ui/ScorePanel.h
    ui/ScorePanel.cpp
)

# 源码中的 #include 均以工程根目录为基准（例如 "core/GameModel.h"）
target_include_directories(OceanBubble PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(OceanBubble PRIVATE Qt5::Widgets)

# 源码为 UTF-8 无 BOM，MSVC 默认按本地代码页解析会把中文注释误读，必须显式 /utf-8
if(MSVC)
    target_compile_options(OceanBubble PRIVATE /utf-8 /W3)
endif()
