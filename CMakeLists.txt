cmake_minimum_required(VERSION 3.21)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" PROJECT_VERSION_CONTENT)
string(STRIP "${PROJECT_VERSION_CONTENT}" PROJECT_VERSION)
project(ReArk VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Concurrent Multimedia LinguistTools)

set(TS_FILES
    "${PROJECT_SOURCE_DIR}/resources/translations/reark_zh_CN.ts"
)

set(HYLE_INSTALL_PREFIX "D:/MilesLi/hyle/install/hyle" CACHE PATH "Installed hyle package prefix")
if(EXISTS "${HYLE_INSTALL_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${HYLE_INSTALL_PREFIX}")
endif()
find_package(hyle CONFIG REQUIRED)

if(WIN32)
    set(APP_ICON_PATH "${PROJECT_SOURCE_DIR}/resources/images/app_icon.ico")
    configure_file(
        "${PROJECT_SOURCE_DIR}/app_icon.rc.in"
        "${CMAKE_BINARY_DIR}/app_icon.rc"
        @ONLY
    )
    set(WIN32_RESOURCES "${CMAKE_BINARY_DIR}/app_icon.rc")
endif()

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(src)
