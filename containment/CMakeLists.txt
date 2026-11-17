add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.latte.containment\")

plasma_install_package(package org.kde.latte.containment)

set(containment_SRCS
    plugin/lattetypes.h
    plugin/types.cpp
    plugin/types.h
    plugin/layoutmanagerapi.h
    plugin/layoutmanager.cpp
    plugin/layoutmanager.h
)

# Types and LayoutManager are registered declaratively via QML_NAMED_ELEMENT;
# qt_add_qml_module generates the plugin class, the registration code and the
# qmldir installed below.
set(LATTE_CONTAINMENT_QML_BUILD_DIR ${CMAKE_BINARY_DIR}/qml/org/kde/latte/private/containment)

qt_add_qml_module(lattecontainmentplugin
    URI org.kde.latte.private.containment
    VERSION 0.1
    CLASS_NAME LatteContainmentPlugin
    PLUGIN_TARGET lattecontainmentplugin
    NO_PLUGIN_OPTIONAL
    OUTPUT_DIRECTORY ${LATTE_CONTAINMENT_QML_BUILD_DIR}
    SOURCES ${containment_SRCS}
)

# QML's Unix plugin loader expects the conventional lib-prefixed filename.
set_target_properties(lattecontainmentplugin PROPERTIES
    PREFIX "lib"
)

target_link_libraries(lattecontainmentplugin PRIVATE
                      latteLogging
                      Qt6::Core
                      Qt6::Qml
                      Qt6::Quick
                      ${LATTE_DECLARATIVE_TARGET}
                      ${LATTE_CONFIGQML_TARGET}
                      KF6::IconThemes
                      Plasma::Plasma
                      Plasma::PlasmaQuick)

# The generated qmltyperegistrations.cpp resolves the module headers via
# __has_include(<layoutmanager.h>), so the plugin/ header directory must be
# on the target include path.
target_include_directories(lattecontainmentplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin)

install(TARGETS lattecontainmentplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/containment)
install(FILES ${LATTE_CONTAINMENT_QML_BUILD_DIR}/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/containment)
# Without this edge, CMake 4.5 cannot infer the generated metadata producer
# and warns during install preparation. Keep the edge version-gated for CMake 3.20.
set(_latte_containment_qmltypes_build_depends)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.5")
    list(APPEND _latte_containment_qmltypes_build_depends BUILD_DEPENDS lattecontainmentplugin)
endif()
install(FILES ${LATTE_CONTAINMENT_QML_BUILD_DIR}/lattecontainmentplugin.qmltypes
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/containment
        ${_latte_containment_qmltypes_build_depends}
        OPTIONAL)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(TARGETS lattecontainmentplugin DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/containment)
    install(FILES ${LATTE_CONTAINMENT_QML_BUILD_DIR}/qmldir DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/containment)
    install(FILES ${LATTE_CONTAINMENT_QML_BUILD_DIR}/lattecontainmentplugin.qmltypes
            DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/containment
            ${_latte_containment_qmltypes_build_depends}
            OPTIONAL)
endif()
