cmake_minimum_required(VERSION 3.16)
project(QtTokenRouter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/core/KeyPoolManager.cpp
    src/core/HttpProxyServer.cpp
    src/ui/MainWindow.cpp
    src/ui/DashboardTab.cpp
    src/ui/KeyManagerTab.cpp
    src/ui/SettingsTab.cpp
    src/ui/AddKeyDialog.cpp
)

set(HEADERS
    src/core/KeyPoolManager.h
    src/core/HttpProxyServer.h
    src/ui/MainWindow.h
    src/ui/DashboardTab.h
    src/ui/KeyManagerTab.h
    src/ui/SettingsTab.h
    src/ui/AddKeyDialog.h
    src/ui/Styles.h
)

add_executable(qt-token-router ${SOURCES} ${HEADERS})

target_link_libraries(qt-token-router PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
