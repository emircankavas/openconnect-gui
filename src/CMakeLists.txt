set(CMAKE_VERBOSE_MAKEFILE ON)

configure_file(
    config.h.in
    config.h
    @ONLY
)

# ---------------------------------------------------------------------------
# ocg-core: connection/business logic, independent of any GUI toolkit.
# Both the Qt Widgets application and the headless ocg-cli link against it.
# ---------------------------------------------------------------------------
add_library(ocg-core STATIC
    cert.cpp
    cryptdata.cpp
    gtdb.cpp
    key.cpp
    keypair.cpp
    keyprompt.cpp
    logger.cpp
    server_storage.cpp
    vpninfo.cpp
    AdPasswordChecker.cpp
)

target_include_directories(ocg-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(ocg-core PUBLIC
    Qt6::Core
)

# Logging. On Linux we pull spdlog in header-only (with a header-only fmt) so
# the produced binaries do not depend on versioned sonames like libfmt.so.9 /
# libspdlog.so.1.12 — those change every distro release, which would make a
# .deb built on one Ubuntu uninstallable on another. Elsewhere keep the normal
# shared-library target.
if(UNIX AND NOT APPLE AND TARGET spdlog::spdlog_header_only)
    if(TARGET fmt::fmt-header-only)
        target_link_libraries(ocg-core PUBLIC fmt::fmt-header-only)
    endif()
    target_compile_definitions(ocg-core PUBLIC
        SPDLOG_HEADER_ONLY
        SPDLOG_FMT_EXTERNAL
    )
    if(TARGET fmt::fmt-header-only)
        target_compile_definitions(ocg-core PUBLIC FMT_HEADER_ONLY)
    endif()
    target_link_libraries(ocg-core PUBLIC spdlog::spdlog_header_only)
else()
    target_link_libraries(ocg-core PUBLIC spdlog::spdlog)
endif()

set_property(SOURCE ${PROJECT_NAME}.cpp PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE timestamp.cpp PROPERTY SKIP_AUTOGEN ON)

# ---------------------------------------------------------------------------
# openconnect-gui: the Qt Widgets application
# ---------------------------------------------------------------------------
set(${PROJECT_NAME}_SRCS
    openconnect-gui.qrc
    common.h
    dialog/MyCertMsgBox.cpp
    dialog/MyInputDialog.cpp
    dialog/MyMsgBox.cpp
    dialog/editdialog.cpp
    dialog/editdialog.ui
    dialog/NewProfileDialog.cpp
    dialog/NewProfileDialog.ui
    dialog/logdialog.cpp
    dialog/logdialog.ui
    dialog/VpnProtocolModel.cpp
    dialog/ProfileIcons.cpp
    dialog/ToggleSwitch.cpp
    dialog/ProfileCard.cpp
    dialog/mainwindow.cpp
    dialog/mainwindow.ui
    main.cpp

    timestamp.cpp

    FileLogger.cpp
    OcSettings.h
)

set_source_files_properties(
    ${PROJECT_NAME}.cpp
    timestamp.cpp
    PROPERTIES
        GENERATED true
)

if(MINGW)
    list(APPEND ${PROJECT_NAME}_RESOURCES
        ${PROJECT_NAME}.rc.in
        ${PROJECT_NAME}.rc
    )

    set_source_files_properties(
       ${PROJECT_NAME}.rc
        PROPERTIES
            GENERATED true
    )
endif()
if(APPLE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/vpnc-script")
        set(VPNC_SCRIPT_TO_BUNDLE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/vpnc-script")
    else()
        set(VPNC_SCRIPT_TO_BUNDLE "${OPENCONNECT_VPNC_SCRIPT}")
    endif()

    list(APPEND ${PROJECT_NAME}_RESOURCES
        Resources/mono_lock.icns
        ${VPNC_SCRIPT_TO_BUNDLE}
    )
    set_source_files_properties(
        Resources/mono_lock.icns
        ${VPNC_SCRIPT_TO_BUNDLE}
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )
endif()

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_RESOURCES}
)

if(MINGW)
    add_definitions(-DSPDLOG_WCHAR_FILENAMES)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/external/vpnc-script.js $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Deploying vpnc-script..."
        VERBATIM
    )

    add_dependencies(${PROJECT_NAME}
        vpnc-scripts-${vpnc-scripts-TAG}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE true
    )

    target_link_libraries (${PROJECT_NAME}
        # system libraries
        wsock32
    )
endif()

#code signing
if(SIGN_EXE STREQUAL "true")
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND sh -c "${CMAKE_SOURCE_DIR}/contrib/sign.sh \"$<TARGET_FILE:${PROJECT_NAME}>\""
        COMMENT "Signing executable ..."
        VERBATIM
    )
endif()

if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    add_dependencies(ocg-core
        openconnect-devel-${openconnect-TAG}
        openconnect-${openconnect-TAG}
    )

    target_link_libraries (ocg-core PUBLIC
        # 3rd party dependencies
        openconnect::gmp
        openconnect::gnutls
        openconnect::hogweed
        openconnect::nettle
        openconnect::openconnect
        openconnect::p11-kit
        openconnect::stoken
        openconnect::xml2
    )
else()
    target_link_libraries (ocg-core PUBLIC
        ${OPENCONNECT_LIBRARIES}
        ${GNUTLS_LIBRARY}
    )
endif()

if(APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${PROJECT_NAME}.plist.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.plist
        @ONLY
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE true
        OUTPUT_NAME ${PRODUCT_NAME_SHORT}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.plist"
        MACOSX_BUNDLE_BUNDLE_NAME ${PRODUCT_NAME_SHORT}
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        MACOSX_BUNDLE_COPYRIGHT "${PRODUCT_NAME_COPYRIGHT_SHORT}"
        MACOSX_BUNDLE_GUI_IDENTIFIER net.openconnect-vpn.gui
        MACOSX_BUNDLE_ICON_FILE mono_lock.icns
        MACOSX_BUNDLE_INFO_STRING ${PRODUCT_NAME_LONG}
    )
endif()


# common stuff to link...
target_link_libraries (${PROJECT_NAME}
    ocg-core

    qt-solutions::qtsingleapplication

    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::StateMachine
)

# The bundled QtSingleApplication is an ExternalProject fetched at build time.
# Its imported target carries a DEPENDS property, but that does not create a
# build-order edge in every CMake/ninja version, so a parallel build can try to
# compile main.cpp (which includes <QtSingleApplication>) before the header is
# installed. Make the dependency explicit.
add_dependencies(${PROJECT_NAME} qt-solutions-${qt-solutions-TAG})

if(UNIX)
    target_link_libraries(ocg-core PUBLIC
        Threads::Threads
    )
    if(APPLE)
        target_link_libraries(ocg-core PUBLIC
            ${SECURITY_LIBRARY}
        )
    endif()
endif()

if(WIN32)
    # ioctlsocket (used by vpninfo) lives in ws2_32; provide it to both the GUI
    # and the headless CLI through the core library.
    target_link_libraries(ocg-core PUBLIC ws2_32)
endif()

# ---------------------------------------------------------------------------
# ocg-cli: headless console front-end (Qt6::Core only, no QtWidgets)
# ---------------------------------------------------------------------------
add_executable(ocg-cli
    cli/main.cpp
    cli/ConsoleUi.cpp
)

target_link_libraries(ocg-cli
    ocg-core
    Qt6::Core
)

# config.h is (re)written by the git_revision target at build time
add_dependencies(ocg-cli git_revision)

# install executable
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} ocg-cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT App
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT App
    )

    # Bundled vpnc-script (carries the split-DNS handling). Installed system-wide
    # so the app finds it at DEFAULT_VPNC_SCRIPT (/usr/share/openconnect-gui).
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/vpnc-script
        DESTINATION ${CMAKE_INSTALL_DATADIR}/openconnect-gui
        COMPONENT App
    )

    # Desktop entry + icon so the app appears in the menu.
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/${PROJECT_NAME}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
        COMPONENT App
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/mono_lock.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME ${PROJECT_NAME}.png
        COMPONENT App
    )
else()
    install(TARGETS ${PROJECT_NAME} ocg-cli
        RUNTIME DESTINATION . COMPONENT App
        BUNDLE DESTINATION . COMPONENT App
        LIBRARY DESTINATION . COMPONENT App
    )
endif()

# fixup the bundle
get_target_property(QT_LIBRARY_DIR Qt6::Core LOCATION)
get_filename_component(QT_LIBRARY_DIR ${QT_LIBRARY_DIR} PATH)
list(APPEND libSearchDirs ${QT_LIBRARY_DIR})

if(MINGW)
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe")
    list(APPEND libSearchDirs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    # Qt Platform Plugin
    install(FILES
        ${_qwindows_dll}
        DESTINATION plugins/platforms
        COMPONENT App
    )
    # Qt schannel plugin
    install(FILES
        ${_schannel_dll}
        DESTINATION plugins/tls
        COMPONENT App
    )
    install(FILES
        ${CMAKE_SOURCE_DIR}/nsis/qt.conf
        DESTINATION .
        COMPONENT App
    )

    set(additionalLib ${_qwindows_dll})
    if(Qt6Core_VERSION VERSION_GREATER_EQUAL "6.0")
        install(FILES
            ${_qwinstyle_dylib}
            DESTINATION plugins/styles
            COMPONENT App
        )
    endif()
elseif(APPLE)
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
    list(APPEND libSearchDirs "")

    # Qt Platform Plugin
    install(FILES
        ${_qcocoa_dylib}
        DESTINATION ${PROJECT_NAME}.app/Contents/PlugIns/platforms
        COMPONENT App
    )
    set(additionalLib \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/platforms/libqcocoa.dylib)
    if(Qt6Core_VERSION VERSION_GREATER_EQUAL "6.0")
        install(FILES
            ${_qmacstyle_dylib}
            DESTINATION ${PROJECT_NAME}.app/Contents/PlugIns/styles
            COMPONENT App
        )
        set(additionalLib
            ${additionalLib}
            \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/styles/libqmacstyle.dylib
        )
    endif()
    install(FILES
        ${CMAKE_SOURCE_DIR}/bundle/qt.conf
        DESTINATION ${PROJECT_NAME}.app/Contents/Resources
        COMPONENT App
    )
else()
    message(STATUS "Nothing to bundle...")
endif()

if(MINGW)
    #wintun
    install(FILES
        ${CMAKE_BINARY_DIR}/bin/wintun.dll
        DESTINATION .
        COMPONENT App
    )
endif()

# Qt runtime (platform plugin, TLS backend, styles) is only bundled into the
# app dir for MinGW/Windows and macOS. On Linux the dependencies come from the
# distro packages declared as .deb Depends, so there is nothing to fix up.
if(MINGW OR APPLE)
    install(CODE "
        include(BundleUtilities)
        if(APPLE)
            set(BU_CHMOD_BUNDLE_ITEMS on)
        endif()
        fixup_bundle(\"${APPS}\" \"${additionalLib}\" \"${libSearchDirs}\")
        " DESTINATION . COMPONENT App
    )
endif()
