cmake_minimum_required(VERSION 3.21)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" PROJECT_VERSION_CONTENT)
string(STRIP "${PROJECT_VERSION_CONTENT}" PROJECT_VERSION)
project(ReArk VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Concurrent Multimedia Network LinguistTools)

set(TS_FILES
    "${PROJECT_SOURCE_DIR}/resources/translations/reark_zh_CN.ts"
)

set(DEFAULT_VCPKG_INSTALLED_PREFIX "D:/tools/vcpkg/installed/x64-windows")
if(DEFINED ENV{VCPKG_INSTALLED_PREFIX} AND NOT "$ENV{VCPKG_INSTALLED_PREFIX}" STREQUAL "")
    set(DEFAULT_VCPKG_INSTALLED_PREFIX "$ENV{VCPKG_INSTALLED_PREFIX}")
elseif(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    set(DEFAULT_VCPKG_INSTALLED_PREFIX "$ENV{VCPKG_ROOT}/installed/x64-windows")
endif()
set(VCPKG_INSTALLED_PREFIX "${DEFAULT_VCPKG_INSTALLED_PREFIX}" CACHE PATH "Installed vcpkg triplet prefix")
if(EXISTS "${VCPKG_INSTALLED_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_PREFIX}")
endif()

set(HYLE_INSTALL_PREFIX "D:/MilesLi/hyle/install/hyle" CACHE PATH "Installed hyle package prefix")
if(EXISTS "${HYLE_INSTALL_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${HYLE_INSTALL_PREFIX}")
endif()
find_package(hyle CONFIG REQUIRED)

set(DEFAULT_WUWE_INSTALL_PREFIX "D:/MilesLi/Wuwe/install")
if(DEFINED ENV{WUWE_INSTALL_PREFIX} AND NOT "$ENV{WUWE_INSTALL_PREFIX}" STREQUAL "")
    set(DEFAULT_WUWE_INSTALL_PREFIX "$ENV{WUWE_INSTALL_PREFIX}")
endif()
set(WUWE_INSTALL_PREFIX "${DEFAULT_WUWE_INSTALL_PREFIX}" CACHE PATH "Installed wuwe package prefix")
if(EXISTS "${WUWE_INSTALL_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${WUWE_INSTALL_PREFIX}")
endif()
find_package(wuwe CONFIG QUIET)
if(wuwe_FOUND)
    message(STATUS "Wuwe Agent runtime enabled")
else()
    message(STATUS "Wuwe Agent runtime not found; smart analysis will show setup guidance")
endif()

find_package(cmark-gfm CONFIG QUIET)
if(cmark-gfm_FOUND)
    find_package(cmark-gfm-extensions CONFIG QUIET)
endif()
if(cmark-gfm_FOUND AND cmark-gfm-extensions_FOUND)
    message(STATUS "cmark-gfm Markdown renderer enabled")
else()
    message(STATUS "cmark-gfm not found; Markdown renderer will use Qt fallback")
endif()

find_package(unofficial-tree-sitter CONFIG QUIET)
if(unofficial-tree-sitter_FOUND)
    message(STATUS "Tree-sitter runtime available for future syntax backends")
else()
    message(STATUS "Tree-sitter runtime not found; syntax highlighting uses built-in lexer")
endif()

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
