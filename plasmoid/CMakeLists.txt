add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.latte.plasmoid\")

plasma_install_package(package org.kde.latte.plasmoid)

find_package(PlasmaActivitiesStats REQUIRED)
find_package(KPipeWire REQUIRED)

# KPipeWire headers need libpipewire-0.3 includes
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

set(tasks_SRCS
    plugin/contextmenuactionsbackend.cpp
    plugin/types.cpp
    plugin/lattetasksplugin.cpp
    plugin/windowscreenshotprovider.cpp
    plugin/pipecaptureworker.cpp
)

add_library(lattetasksplugin SHARED ${tasks_SRCS})

target_include_directories(lattetasksplugin PRIVATE
    /usr/include/KPipeWire
)

target_link_libraries(lattetasksplugin
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Qml
                      Qt6::Widgets
                      Qt6::DBus
                      KF6::I18n
                      KF6::KIOCore
                      KF6::KIOFileWidgets
                      KF6::KIOWidgets
                      Plasma::ActivitiesStats
                      Plasma::Plasma
                      Plasma::PlasmaQuick
                      KPipeWire
                      PkgConfig::PIPEWIRE)
                      
install(TARGETS lattetasksplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(TARGETS lattetasksplugin DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
    install(FILES plugin/qmldir DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
endif()
