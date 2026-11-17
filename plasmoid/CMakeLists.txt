add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.latte.plasmoid\")

plasma_install_package(package org.kde.latte.plasmoid)

find_package(PlasmaActivitiesStats REQUIRED)

add_executable(latte-dock-ng-preview preview/main.cpp)
configure_file(org.kde.latte-dock.preview.desktop.cmake org.kde.latte-dock.preview.desktop)
qt_add_resources(latte-dock-ng-preview preview_resources
    PREFIX "/preview" BASE "preview"
    FILES preview/Preview.qml preview/Capture.qml)
target_link_libraries(latte-dock-ng-preview PRIVATE
    Qt6::Widgets Qt6::Quick Qt6::Qml Qt6::Gui
    Plasma::Plasma LayerShellQt::Interface KF6::IconThemes KF6::I18n KF6::I18nQml)
if(LATTE_LAYERSHELL_HAS_SET_SCREEN)
    target_compile_definitions(latte-dock-ng-preview PRIVATE LATTE_LAYERSHELL_HAS_SET_SCREEN)
endif()
if(LATTE_LAYERSHELL_HAS_DESIRED_SIZE)
    target_compile_definitions(latte-dock-ng-preview PRIVATE LATTE_LAYERSHELL_HAS_DESIRED_SIZE)
endif()
if(LATTE_LAYERSHELL_HAS_ACTIVATE_ON_SHOW)
    target_compile_definitions(latte-dock-ng-preview PRIVATE LATTE_LAYERSHELL_HAS_ACTIVATE_ON_SHOW)
endif()
install(TARGETS latte-dock-ng-preview DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.latte-dock.preview.desktop
        DESTINATION ${KDE_INSTALL_APPDIR})

set(tasks_SRCS
    plugin/previewprocess.cpp
    plugin/previewprocess.h
    plugin/windowviewbackend.cpp
    plugin/windowviewbackend.h
    plugin/contextmenuactionsbackend.cpp
    plugin/contextmenuactionsbackend.h
    plugin/types.cpp
    plugin/types.h
)

# Task types and backends are registered declaratively via
# QML_NAMED_ELEMENT; qt_add_qml_module generates the plugin class, the
# registration code and the qmldir installed below.
set(LATTE_TASKS_QML_BUILD_DIR ${CMAKE_BINARY_DIR}/qml/org/kde/latte/private/tasks)

qt_add_qml_module(lattetasksplugin
    URI org.kde.latte.private.tasks
    VERSION 0.1
    CLASS_NAME LatteTasksPlugin
    PLUGIN_TARGET lattetasksplugin
    NO_PLUGIN_OPTIONAL
    OUTPUT_DIRECTORY ${LATTE_TASKS_QML_BUILD_DIR}
    SOURCES ${tasks_SRCS}
)

# QML's Unix plugin loader expects the conventional lib-prefixed filename.
set_target_properties(lattetasksplugin PROPERTIES
    PREFIX "lib"
)

target_link_libraries(lattetasksplugin PRIVATE
                      Qt6::DBus
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Qml
                      Qt6::Widgets
                      KF6::I18n
                      KF6::KIOCore
                      KF6::KIOFileWidgets
                      KF6::KIOWidgets
                      Plasma::ActivitiesStats
                      Plasma::Plasma
                      Plasma::PlasmaQuick)

# The generated qmltyperegistrations.cpp resolves the module headers via
# __has_include(<types.h>), so the plugin/ header directory must be on the
# target include path.
target_include_directories(lattetasksplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin)

install(TARGETS lattetasksplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
# Without this edge, CMake 4.5 cannot infer the generated metadata producer
# and warns during install preparation. Keep the edge version-gated for CMake 3.20.
set(_latte_tasks_qmltypes_build_depends)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.5")
    list(APPEND _latte_tasks_qmltypes_build_depends BUILD_DEPENDS lattetasksplugin)
endif()
install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/lattetasksplugin.qmltypes
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks
        ${_latte_tasks_qmltypes_build_depends}
        OPTIONAL)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(TARGETS lattetasksplugin DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
    install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/qmldir DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
    install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/lattetasksplugin.qmltypes
            DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks
            ${_latte_tasks_qmltypes_build_depends}
            OPTIONAL)
endif()
