cmake_minimum_required(VERSION 3.16)

project(LinePuppyLLK VERSION 0.1 LANGUAGES CXX)

#【Qt必须配置】开启MOC、UIC、RCC自动处理（信号槽、UI文件、资源文件必备）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Multimedia MultimediaWidgets Sql)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appLinePuppyLLK
    # 头文件 (.h)
    "Front_end/Header Files/stdafx.h"
    "Front_end/Header Files/gamedialog.h"
    "Front_end/Header Files/custommodesettingsdialog.h"
    "Front_end/Header Files/settingsdialog.h"
    "Front_end/Header Files/llk.h"
    "Front_end/Header Files/llkdialog.h"
    "Front_end/Header Files/levelselectdialog.h"
    "Front_end/Header Files/logindialog.h"
    "Front_end/Header Files/registerdialog.h"
    "Front_end/Header Files/rankdialog.h"
    "Front_end/Header Files/helpdialog.h"
    "Back_end/Header Files/gamecommon.h"
    "Back_end/Header Files/gamecontrol.h"
    "Back_end/Header Files/llkappsettings.h"
    "Back_end/Header Files/dbconfighelper.h"
    "Back_end/Header Files/gameexception.h"
    "Back_end/Header Files/gamelogic.h"
    "Back_end/Header Files/userauthservice.h"
    "Back_end/Header Files/userprogressservice.h"
    # 源文件 (.cpp)
    "Front_end/Source Files/gamedialog.cpp"
    "Front_end/Source Files/gamedialog_input.cpp"
    "Front_end/Source Files/gamedialog_paint.cpp"
    "Front_end/Source Files/custommodesettingsdialog.cpp"
    "Front_end/Source Files/settingsdialog.cpp"
    "Front_end/Source Files/llk.cpp"
    "Front_end/Source Files/llkdialog.cpp"
    "Front_end/Source Files/levelselectdialog.cpp"
    "Front_end/Source Files/logindialog.cpp"
    "Front_end/Source Files/registerdialog.cpp"
    "Front_end/Source Files/rankdialog.cpp"
    "Front_end/Source Files/helpdialog.cpp"
    "Back_end/Source Files/llkappsettings.cpp"
    "Back_end/Source Files/dbconfighelper.cpp"
    "Back_end/Source Files/gameexception.cpp"
    "Back_end/Source Files/gamelogic.cpp"
    "Back_end/Source Files/userauthservice.cpp"
    "Back_end/Source Files/userprogressservice.cpp"
    # 资源文件 (.qrc)
    "Resource Files/res.qrc"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dbconfig.ini"
    "${CMAKE_CURRENT_BINARY_DIR}/dbconfig.ini"
    COPYONLY
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
# 设置目标属性（去掉 macOS/iOS 相关，保留 Windows 配置）
set_target_properties(appLinePuppyLLK PROPERTIES
    WIN32_EXECUTABLE TRUE  # Windows 下去掉控制台黑框（如果需要调试黑框可设为 FALSE）
)

target_include_directories(appLinePuppyLLK
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Front_end/Header Files"
        "${CMAKE_CURRENT_SOURCE_DIR}/Back_end/Header Files"
)

target_link_libraries(appLinePuppyLLK
    PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Multimedia Qt6::MultimediaWidgets Qt6::Sql
)

install(TARGETS appLinePuppyLLK
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
