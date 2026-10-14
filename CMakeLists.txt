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
        Network
        Sql
        SerialBus
        SerialPort
)
qt_standard_project_setup()

set(PROJECT_SOURCES
    main.cpp
    ECG_monitor.rc
    Resource.qrc

    #第三方插件
    3rdparty/qcustomplot/qcustomplot.h
    3rdparty/qcustomplot/qcustomplot.cpp
    
    #ui界面
    #主界面
    ui/MainWindow.ui
    ui/MainWindow.h
    ui/MainWindow.cpp
    #设置对话框
    ui/SettingDialog.ui
    ui/SettingDialog.h
    ui/SettingDialog.cpp

    #网络
    Network/StationData.h
    Network/NetworkManager.h
    Network/NetworkManager.cpp

    #数据库
    Database/DatabaseManager.h
    Database/DatabaseManager.cpp

    #配置管理器（全局单例）
    ConfigManager/ConfigManager.h
    ConfigManager/ConfigManager.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Database
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::PrintSupport
        Qt::Network
        Qt::Sql
        Qt::SerialBus
        Qt::SerialPort
)

