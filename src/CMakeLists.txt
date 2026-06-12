set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    core/AppInitializer.cpp
    core/AppInitializer.h
    core/ApplicationController.cpp
    core/ApplicationController.h
    core/BuildInfoProvider.cpp
    core/BuildInfoProvider.h
    core/HyleDecompiler.cpp
    core/HyleDecompiler.h
    core/PerformanceTrace.cpp
    core/PerformanceTrace.h
    core/ResourcePreviewProvider.cpp
    core/ResourcePreviewProvider.h
    core/Translator.cpp
    core/Translator.h
    core/WindowChrome.cpp
    core/WindowChrome.h
    controller/DecompilerController.cpp
    controller/DecompilerController.h
    controller/AgentSettings.cpp
    controller/AgentSettings.h
    controller/AgentKnowledgeController.cpp
    controller/AgentKnowledgeController.h
    controller/AgentController.cpp
    controller/AgentController.h
    controller/LanguageController.cpp
    controller/LanguageController.h
    controller/SettingsController.cpp
    controller/SettingsController.h
    controller/UpdateController.cpp
    controller/UpdateController.h
    model/AgentMessageModel.cpp
    model/AgentMessageModel.h
    model/DocumentContent.h
    model/OpenFileTabsModel.cpp
    model/OpenFileTabsModel.h
    model/RecentFilesModel.cpp
    model/RecentFilesModel.h
    model/HexDocumentModel.cpp
    model/HexDocumentModel.h
    model/SourceTreeModel.cpp
    model/SourceTreeModel.h
    presentation/CodeTheme.cpp
    presentation/CodeTheme.h
    presentation/CodeEditorItem.cpp
    presentation/CodeEditorItem.h
    presentation/HexViewerItem.cpp
    presentation/HexViewerItem.h
    presentation/MarkdownRenderer.cpp
    presentation/MarkdownRenderer.h
    presentation/SyntaxThemeProvider.cpp
    presentation/SyntaxThemeProvider.h
    ${WIN32_RESOURCES}
)

set_source_files_properties(ui/qml/main.qml PROPERTIES
    QT_RESOURCE_ALIAS main.qml
)

qt_add_qml_module(${PROJECT_NAME}
    URI "com.reark.app"
    VERSION 1.0
    RESOURCE_PREFIX /
    IMPORTS
        ReArk
    QML_FILES
        ui/qml/main.qml
)

qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    LUPDATE_OPTIONS -no-obsolete
)

set(REARK_BUILD_INFO_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/BuildInfo.h")
add_custom_target(reark_build_info ALL
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE="${REARK_BUILD_INFO_HEADER}"
        -P "${CMAKE_SOURCE_DIR}/cmake/WriteBuildInfo.cmake"
    BYPRODUCTS "${REARK_BUILD_INFO_HEADER}"
    COMMENT "Writing build metadata"
)
add_dependencies(${PROJECT_NAME} reark_build_info)

add_subdirectory(ui/qml/ReArk)

qt_add_resources(${PROJECT_NAME} "images"
    PREFIX "/images"
    BASE "${CMAKE_SOURCE_DIR}/resources/images"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/images/app_icon.ico"
        "${CMAKE_SOURCE_DIR}/resources/images/app_icon.png"
)

qt_add_resources(${PROJECT_NAME} "icons"
    PREFIX "/icons"
    BASE "${CMAKE_SOURCE_DIR}/resources/icons"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/icons/chevron-down.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/chevron-up.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/close-small.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/arrow-up.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/check.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/copy.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/gem.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/paperclip.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/fontawesome/square-plus.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/find-match-case.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/find-regex.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/find-whole-word.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/folder-open-white.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/github.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/github-white.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/issue.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/issue-white.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/trash-muted.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/upload-muted.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/website.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/website-white.svg"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Network
    hyle::hyle
    reark_qml_module
    reark_qml_moduleplugin
)

if(wuwe_FOUND)
    get_target_property(REARK_WUWE_INCLUDE_DIRS wuwe::wuwe INTERFACE_INCLUDE_DIRECTORIES)
    if(REARK_WUWE_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${REARK_WUWE_INCLUDE_DIRS})
    endif()
    get_target_property(REARK_WUWE_IMPORTED_CONFIGS wuwe::wuwe IMPORTED_CONFIGURATIONS)
    get_target_property(REARK_WUWE_IMPORTED_LOCATION_DEBUG wuwe::wuwe IMPORTED_LOCATION_DEBUG)
    get_target_property(REARK_WUWE_IMPORTED_LOCATION_RELEASE wuwe::wuwe IMPORTED_LOCATION_RELEASE)
    if(REARK_WUWE_IMPORTED_CONFIGS MATCHES "DEBUG"
        AND REARK_WUWE_IMPORTED_LOCATION_DEBUG
        AND NOT REARK_WUWE_IMPORTED_LOCATION_DEBUG STREQUAL REARK_WUWE_IMPORTED_LOCATION_RELEASE)
        set(REARK_WUWE_CONFIG_EXPRESSION "1")
        message(STATUS "Wuwe Agent runtime enabled for all imported configurations")
    else()
        set(REARK_WUWE_CONFIG_EXPRESSION "$<NOT:$<CONFIG:Debug>>")
        message(STATUS "Wuwe Agent runtime disabled for Debug because the installed package exports no Debug library")
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "$<${REARK_WUWE_CONFIG_EXPRESSION}:wuwe::wuwe>"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<${REARK_WUWE_CONFIG_EXPRESSION}:REARK_HAS_WUWE=1>"
    )
endif()

if(cmark-gfm_FOUND AND cmark-gfm-extensions_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libcmark-gfm
        libcmark-gfm-extensions
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE REARK_HAS_CMARK_GFM=1)
endif()

if(unofficial-tree-sitter_FOUND)
    set(REARK_TREE_SITTER_GRAMMAR_ROOT "${CMAKE_SOURCE_DIR}/third_party/tree-sitter-grammars")
    set(REARK_TREE_SITTER_JAVASCRIPT_ROOT "${REARK_TREE_SITTER_GRAMMAR_ROOT}/javascript")
    set(REARK_TREE_SITTER_TYPESCRIPT_ROOT "${REARK_TREE_SITTER_GRAMMAR_ROOT}/typescript/typescript")

    if(EXISTS "${REARK_TREE_SITTER_JAVASCRIPT_ROOT}/src/parser.c"
        AND EXISTS "${REARK_TREE_SITTER_TYPESCRIPT_ROOT}/src/parser.c")
        add_library(reark_tree_sitter_javascript STATIC
            "${REARK_TREE_SITTER_JAVASCRIPT_ROOT}/src/parser.c"
            "${REARK_TREE_SITTER_JAVASCRIPT_ROOT}/src/scanner.c"
        )
        target_include_directories(reark_tree_sitter_javascript PRIVATE
            "${REARK_TREE_SITTER_JAVASCRIPT_ROOT}/src"
        )
        set_target_properties(reark_tree_sitter_javascript PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
        )

        add_library(reark_tree_sitter_typescript STATIC
            "${REARK_TREE_SITTER_TYPESCRIPT_ROOT}/src/parser.c"
            "${REARK_TREE_SITTER_TYPESCRIPT_ROOT}/src/scanner.c"
        )
        target_include_directories(reark_tree_sitter_typescript PRIVATE
            "${REARK_TREE_SITTER_TYPESCRIPT_ROOT}/src"
        )
        set_target_properties(reark_tree_sitter_typescript PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
        )

        target_link_libraries(${PROJECT_NAME} PRIVATE
            reark_tree_sitter_javascript
            reark_tree_sitter_typescript
        )
        if(MSVC)
            foreach(_reark_tree_sitter_grammar_target
                reark_tree_sitter_javascript
                reark_tree_sitter_typescript)
                target_compile_options(${_reark_tree_sitter_grammar_target} PRIVATE
                    /bigobj
                    /wd4244
                    /wd4267
                )
            endforeach()
        endif()
        message(STATUS "Tree-sitter JavaScript/TypeScript grammars enabled")
        target_compile_definitions(${PROJECT_NAME} PRIVATE REARK_HAS_TREE_SITTER_GRAMMARS=1)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::tree-sitter::tree-sitter)
    target_compile_definitions(${PROJECT_NAME} PRIVATE REARK_HAS_TREE_SITTER=1)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/presentation>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

qt_import_qml_plugins(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    REARK_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Crypt32)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )

    get_target_property(_qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
    find_program(REARK_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${_qt_bin_dir}"
        REQUIRED
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${REARK_WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            $<$<CONFIG:Debug>:--debug>
            $<$<NOT:$<CONFIG:Debug>>:--release>
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/ui/qml"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Deploying Qt runtime plugins"
    )

    if(TARGET OpenSSL::Crypto)
        get_target_property(_openssl_crypto_runtime
            OpenSSL::Crypto IMPORTED_LOCATION)
        if(NOT _openssl_crypto_runtime)
            get_target_property(_openssl_crypto_runtime
                OpenSSL::Crypto IMPORTED_LOCATION_RELEASE)
        endif()
        if(_openssl_crypto_runtime)
            get_filename_component(_openssl_crypto_runtime_ext
                "${_openssl_crypto_runtime}" EXT)
            if(NOT _openssl_crypto_runtime_ext STREQUAL ".dll")
                set(_openssl_crypto_implib "${_openssl_crypto_runtime}")
                unset(_openssl_crypto_runtime)
            endif()
        endif()
        if(NOT _openssl_crypto_runtime)
            if(NOT _openssl_crypto_implib)
                get_target_property(_openssl_crypto_implib
                    OpenSSL::Crypto IMPORTED_IMPLIB)
            endif()
            if(NOT _openssl_crypto_implib)
                get_target_property(_openssl_crypto_implib
                    OpenSSL::Crypto IMPORTED_IMPLIB_RELEASE)
            endif()
            if(_openssl_crypto_implib)
                get_filename_component(_openssl_root
                    "${_openssl_crypto_implib}/../.." ABSOLUTE)
                file(GLOB _openssl_crypto_candidates
                    "${_openssl_root}/bin/libcrypto-*.dll"
                    "${_openssl_root}/bin/libeay32.dll")
                if(_openssl_crypto_candidates)
                    list(GET _openssl_crypto_candidates 0
                        _openssl_crypto_runtime)
                endif()
            endif()
        endif()

        if(_openssl_crypto_runtime)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_openssl_crypto_runtime}"
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endif()
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:MinSizeRel>:/O1>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
endif()
