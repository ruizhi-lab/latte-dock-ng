set(latteprivateappplugin_SRCS
    contextmenulayerquickitem.cpp
    contextmenulayerquickitem.h
    interfaces.cpp
    interfaces.h
    ../plasma/extended/backgroundtracker.cpp
    ../plasma/extended/backgroundtracker.h
)

set(LATTE_PRIVATE_APP_QML_BUILD_DIR ${CMAKE_BINARY_DIR}/qml/org/kde/latte/private/app)

# KDE's module defaults require all symbols to resolve at link time.  The
# application-facing plugin deliberately resolves View, Storage and
# BackgroundCache from the exporting latte-dock-ng host at plugin load time.
string(REGEX REPLACE "(^| )-Wl,--no-undefined( |$)" " "
       CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")

qt_add_qml_module(latteprivateappplugin
    URI org.kde.latte.private.app
    VERSION 0.1
    CLASS_NAME LattePrivateAppPlugin
    PLUGIN_TARGET latteprivateappplugin
    NO_PLUGIN_OPTIONAL
    OUTPUT_DIRECTORY ${LATTE_PRIVATE_APP_QML_BUILD_DIR}
    SOURCES ${latteprivateappplugin_SRCS}
)

# Export the type definitions because the plugin shares its QML type symbols
# with the exporting latte-dock-ng host process.
set_target_properties(latteprivateappplugin PROPERTIES
    CXX_VISIBILITY_PRESET default
    PREFIX "lib"
)

target_link_libraries(latteprivateappplugin PRIVATE
    latteLogging
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    KF6::Package
    KF6::Service
    KF6::Svg
    KF6::XmlGui
    Plasma::Activities
    Plasma::Plasma
    Plasma::PlasmaQuick
)

target_include_directories(latteprivateappplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../plasma/extended
)

install(TARGETS latteprivateappplugin
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/app)
install(FILES ${LATTE_PRIVATE_APP_QML_BUILD_DIR}/qmldir
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/app)
# Without this edge, CMake 4.5 cannot infer the generated metadata producer
# and warns during install preparation. Keep the edge version-gated for CMake 3.20.
set(_latte_private_app_qmltypes_build_depends)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.5")
    list(APPEND _latte_private_app_qmltypes_build_depends BUILD_DEPENDS latteprivateappplugin)
endif()
install(FILES ${LATTE_PRIVATE_APP_QML_BUILD_DIR}/latteprivateappplugin.qmltypes
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/app
        ${_latte_private_app_qmltypes_build_depends}
        OPTIONAL)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(TARGETS latteprivateappplugin DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/app)
    install(FILES ${LATTE_PRIVATE_APP_QML_BUILD_DIR}/qmldir
            DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/app)
    install(FILES ${LATTE_PRIVATE_APP_QML_BUILD_DIR}/latteprivateappplugin.qmltypes
            DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/app
            ${_latte_private_app_qmltypes_build_depends}
            OPTIONAL)
endif()
