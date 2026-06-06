set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    core/AppInitializer.cpp
    core/AppInitializer.h
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
    controller/LanguageController.cpp
    controller/LanguageController.h
    model/DocumentContent.h
    model/OpenFileTabsModel.cpp
    model/OpenFileTabsModel.h
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
        "${CMAKE_SOURCE_DIR}/resources/icons/email.svg"
        "${CMAKE_SOURCE_DIR}/resources/icons/github.svg"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Concurrent
    Qt6::Multimedia
    hyle::hyle
    reark_qml_module
    reark_qml_moduleplugin
)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/presentation>
)

qt_import_qml_plugins(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    REARK_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

if(WIN32)
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
