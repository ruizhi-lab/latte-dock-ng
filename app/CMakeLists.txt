set(lattedock-app_SRCS
    alternativeshelper.cpp
    apptypes.cpp
    infoview.cpp
    lattecorona.cpp
    screenpool.cpp
    primaryoutputwatcher.cpp
    main.cpp    
    coretypes.h
)

add_subdirectory(data)
add_subdirectory(declarativeimports)
add_subdirectory(indicator)
add_subdirectory(layout)
add_subdirectory(layouts)
add_subdirectory(package)
add_subdirectory(plasma/extended)
add_subdirectory(settings)
add_subdirectory(settings/generic)
add_subdirectory(settings/actionsdialog)
add_subdirectory(settings/detailsdialog)
add_subdirectory(settings/detailsdialog/delegates)
add_subdirectory(settings/exporttemplatedialog)
add_subdirectory(settings/exporttemplatedialog/delegates)
add_subdirectory(settings/screensdialog)
add_subdirectory(settings/screensdialog/delegates)
add_subdirectory(settings/settingsdialog)
add_subdirectory(settings/settingsdialog/delegates)
add_subdirectory(settings/viewsdialog)
add_subdirectory(settings/viewsdialog/delegates)
add_subdirectory(shortcuts)
add_subdirectory(templates)
add_subdirectory(tools)
add_subdirectory(view)
add_subdirectory(view/helpers)
add_subdirectory(view/indicator)
add_subdirectory(view/settings)
add_subdirectory(view/windowstracker)
add_subdirectory(wm)
add_subdirectory(wm/tracker)

set(latte_dbusXML dbus/org.kde.LatteDock.xml)
qt_add_dbus_adaptor(lattedock-app_SRCS ${latte_dbusXML} lattecorona.h Latte::Corona lattedockadaptor)

ki18n_wrap_ui(lattedock-app_SRCS settings/actionsdialog/actionsdialog.ui)
ki18n_wrap_ui(lattedock-app_SRCS settings/detailsdialog/detailsdialog.ui)
ki18n_wrap_ui(lattedock-app_SRCS settings/exporttemplatedialog/exporttemplatedialog.ui)
ki18n_wrap_ui(lattedock-app_SRCS settings/screensdialog/screensdialog.ui)
ki18n_wrap_ui(lattedock-app_SRCS settings/settingsdialog/settingsdialog.ui)
ki18n_wrap_ui(lattedock-app_SRCS settings/viewsdialog/viewsdialog.ui)

ecm_add_qtwayland_client_protocol(lattedock-app_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
    BASENAME kde-primary-output-v1
)

add_executable(latte-dock-ng ${lattedock-app_SRCS})

include(FakeTarget.cmake)

target_link_libraries(latte-dock-ng
    Qt6::DBus
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Plasma::Activities
    KF6::Archive
    KF6::CoreAddons
    KF6::Crash
    KF6::DBusAddons
    ${LATTE_DECLARATIVE_TARGET}
    KF6::GuiAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::IconThemes
    KF6::IconWidgets
    KF6::KIOWidgets
    KF6::Notifications
    KF6::Package
    ${LATTE_CONFIGQML_TARGET}
    ${LATTE_NEWSTUFF_TARGET}
    ${LATTE_QUICKADDONS_TARGET}
    ${LATTE_SVG_TARGET}
    Plasma::Plasma
    Plasma::PlasmaQuick
    ${LATTE_WAYLANDCLIENT_TARGET}
    KF6::WindowSystem
    KF6::XmlGui
    Wayland::Client
)

configure_file(org.kde.latte-dock.desktop.cmake org.kde.latte-dock.desktop)
configure_file(org.kde.latte-dock.appdata.xml.cmake org.kde.latte-dock.appdata.xml)

install(TARGETS latte-dock-ng ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.latte-dock.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.latte-dock.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES dbus/org.kde.LatteDock.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
install(FILES lattedock.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
install(FILES latte-layouts.knsrc DESTINATION  ${KDE_INSTALL_KNSRCDIR})
install(FILES latte-indicators.knsrc DESTINATION  ${KDE_INSTALL_KNSRCDIR})


add_subdirectory(packageplugins)
