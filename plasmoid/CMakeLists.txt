add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.latte.plasmoid\")

plasma_install_package(package org.kde.latte.plasmoid)

find_package(PlasmaActivitiesStats REQUIRED)

set(tasks_SRCS
    plugin/contextmenuactionsbackend.cpp
    plugin/types.cpp
    plugin/lattetasksplugin.cpp
)

add_library(lattetasksplugin SHARED ${tasks_SRCS})

target_link_libraries(lattetasksplugin
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
                      
install(TARGETS lattetasksplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
