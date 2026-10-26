add_definitions(-DTRANSLATION_DOMAIN=\"plasma_containmentactions_lattecontextmenu\")

set(contextmenu_SRCS
    generictools.cpp
    layoutmenuitemwidget.cpp
    menu.cpp
)

add_library(plasma_containmentactions_lattecontextmenu MODULE ${contextmenu_SRCS})

# Plasma's PluginLoader looks up containmentactions plugins by relative path
# "plasma/containmentactions/<plugin id>", and KF6 derives the plugin id from
# the file name, so the installed .so must be named org.kde.latte.contextmenu,
# the id the layouts store. Other built-in containmentactions in Plasma
# (org.kde.contextmenu.so, org.kde.applauncher.so) follow the same convention.
# Without this rename Plasma silently fails to load the plugin and right-click
# on the dock has no menu. The metadata JSON therefore carries no explicit
# KPlugin.Id; KPluginMetaData logs an info message when it does.
set_target_properties(plasma_containmentactions_lattecontextmenu PROPERTIES
    OUTPUT_NAME "org.kde.latte.contextmenu"
    PREFIX ""
)

target_link_libraries(plasma_containmentactions_lattecontextmenu
                        latteLogging
                        Qt6::DBus
                        Qt6::Widgets
                        KF6::I18n
                        KF6::XmlGui
                        Plasma::Plasma)

install(TARGETS plasma_containmentactions_lattecontextmenu DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/containmentactions)
