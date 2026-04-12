cmake_minimum_required(VERSION 3.16)

project(Xiasl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

find_path(QSCINTILLA_INCLUDE_DIR NAMES Qsci/qsciscintilla.h)
find_library(QSCINTILLA_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6 qscintilla2_qt5 qscintilla2-qt5 qscintilla2)

if(NOT QSCINTILLA_INCLUDE_DIR OR NOT QSCINTILLA_LIBRARY)
    message(FATAL_ERROR "System QScintilla not found. Please install QScintilla development package.")
endif()

set(PROJECT_SOURCES
    src/MyTabBar.cpp
    src/MyTabPage.cpp
    src/MyTabPopup.cpp
    src/MyTabWidget.cpp
    src/autoupdatedialog.cpp
    src/dlgdecompile.cpp
    src/dlgpreferences.cpp
    src/filesystemwatcher.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/methods.cpp
    src/minidialog.cpp
    src/myapp.cpp
    src/recentfiles.cpp
)

set(PROJECT_HEADERS
    include/MyTabBar.h
    include/MyTabPage.h
    include/MyTabPopup.h
    include/autoupdatedialog.h
    include/dlgdecompile.h
    include/dlgpreferences.h
    include/filesystemwatcher.h
    include/mainwindow.h
    include/methods.h
    include/minidialog.h
    include/myapp.h
    include/mytabwidget.h
    include/recentfiles.h
)

set(PROJECT_UIS
    ui/autoupdatedialog.ui
    ui/dlgdecompile.ui
    ui/dlgpreferences.ui
    ui/mainwindow.ui
    ui/minidialog.ui
)

set(PLATFORM_SOURCES)
if(APPLE)
    list(APPEND PLATFORM_SOURCES src/OSXHideTitleBar.mm include/OSXHideTitleBar.h)
endif()

if(WIN32)
    list(APPEND PLATFORM_SOURCES myapp.rc)
endif()

if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${PROJECT_UIS}
        ${PLATFORM_SOURCES}
        my.qrc
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
else()
    add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${PROJECT_UIS}
        ${PLATFORM_SOURCES}
        my.qrc
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${QSCINTILLA_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    "${QSCINTILLA_LIBRARY}"
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Foundation" "-framework Cocoa")
endif()
