configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-latte-lib.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-latte-lib.h)

set(lattecoreplugin_SRCS
    lattecoreplugin.cpp
    dialog.cpp
    environment.cpp
    iconitem.cpp
    quickwindowsystem.cpp
    tools.cpp
    types.h
)

add_library(lattecoreplugin SHARED ${lattecoreplugin_SRCS})

target_link_libraries(lattecoreplugin
    Qt6::Quick
    Qt6::Qml
    KF6::CoreAddons
    KF6::WindowSystem
    ${LATTE_SVG_TARGET}
    Plasma::Plasma
    Plasma::PlasmaQuick
    ${LATTE_QUICKADDONS_TARGET}
    KF6::IconThemes
    KF6::GuiAddons
)

install(TARGETS lattecoreplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/core)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/core)
