cmake_minimum_required(VERSION 3.16)
project(ECG_monitor LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#开启 Qt 资源自动处理
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
        PrintSupport
)
qt_standard_project_setup()

set(PROJECT_SOURCES
    main.cpp
    ECG_monitor.rc
    Resource.qrc

    3rdparty/qcustomplot/qcustomplot.h
    3rdparty/qcustomplot/qcustomplot.cpp

    ui/MainWindow.ui
    ui/MainWindow.h
    ui/MainWindow.cpp
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE FALSE
)

#设置头文件搜索路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qcustomplot
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::PrintSupport
)

