set(latte_autotest_targets)

function(latte_add_test _test_name)
    add_test(NAME ${_test_name} COMMAND ${_test_name})
    list(APPEND latte_autotest_targets ${_test_name})
    set(latte_autotest_targets ${latte_autotest_targets} PARENT_SCOPE)
endfunction()

function(latte_add_offscreen_test _test_name)
    latte_add_test(${_test_name})
    set(latte_autotest_targets ${latte_autotest_targets} PARENT_SCOPE)
    set_tests_properties(${_test_name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

set(dataunittest_SRCS
    dataunittest.cpp
    ../app/data/activitydata.cpp
    ../app/data/appletdata.cpp
    ../app/data/errordata.cpp
    ../app/data/errorinformationdata.cpp
    ../app/data/genericbasictable.cpp
    ../app/data/genericdata.cpp
    ../app/data/generictable.cpp
    ../app/data/layoutdata.cpp
    ../app/data/layoutcolordata.cpp
    ../app/data/layouticondata.cpp
    ../app/data/layoutstable.cpp
    ../app/data/preferencesdata.cpp
    ../app/data/screendata.cpp
    ../app/data/uniqueidinfo.cpp
    ../app/data/viewdata.cpp
    ../app/data/viewstable.cpp
    ../app/tools/commontools.cpp
)

add_executable(dataunittest EXCLUDE_FROM_ALL ${dataunittest_SRCS})

target_include_directories(dataunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/tools
)

target_link_libraries(dataunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore
    Plasma::Plasma
)

latte_add_test(dataunittest)

set(modelunittest_SRCS
    modelunittest.cpp
    ../app/settings/exporttemplatedialog/appletsmodel.cpp
    ../app/settings/screensdialog/screensmodel.cpp
    ../app/data/activitydata.cpp
    ../app/data/appletdata.cpp
    ../app/data/errordata.cpp
    ../app/data/errorinformationdata.cpp
    ../app/data/genericdata.cpp
    ../app/data/generictable.cpp
    ../app/data/layoutdata.cpp
    ../app/data/screendata.cpp
    ../app/data/uniqueidinfo.cpp
    ../app/data/viewdata.cpp
    ../app/data/viewstable.cpp
    ../app/tools/commontools.cpp
)

add_executable(modelunittest EXCLUDE_FROM_ALL ${modelunittest_SRCS})

target_include_directories(modelunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/exporttemplatedialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/screensdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/tools
)

target_link_libraries(modelunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore
    KF6::I18n
    Plasma::Plasma
)

latte_add_test(modelunittest)

set(coreunittest_SRCS
    coreunittest.cpp
    ../declarativeimports/core/environment.cpp
    ../declarativeimports/core/extras.cpp
    ../declarativeimports/core/quickwindowsystem.cpp
    ../declarativeimports/core/tools.cpp
)

add_executable(coreunittest EXCLUDE_FROM_ALL ${coreunittest_SRCS})

target_include_directories(coreunittest PRIVATE
    ${CMAKE_BINARY_DIR}/declarativeimports/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../declarativeimports/core
)

target_link_libraries(coreunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::IconThemes
    Plasma::Plasma
)

latte_add_offscreen_test(coreunittest)

set(corecomponentstest_SRCS
    corecomponentstest.cpp
    ../app/apptypes.cpp
    ../app/plasma/extended/backgroundcache.cpp
    ../app/plasma/extended/backgroundtracker.cpp
    ../app/plasma/extended/screenpool.cpp
    ../app/primaryoutputwatcher.cpp
    ../app/screenpool.cpp
    ../app/data/activitydata.cpp
    ../app/data/appletdata.cpp
    ../app/data/errordata.cpp
    ../app/data/errorinformationdata.cpp
    ../app/data/genericbasictable.cpp
    ../app/data/genericdata.cpp
    ../app/data/generictable.cpp
    ../app/data/layoutdata.cpp
    ../app/data/layoutcolordata.cpp
    ../app/data/layouticondata.cpp
    ../app/data/layoutstable.cpp
    ../app/data/preferencesdata.cpp
    ../app/data/screendata.cpp
    ../app/data/uniqueidinfo.cpp
    ../app/data/viewdata.cpp
    ../app/data/viewstable.cpp
    ../app/tools/commontools.cpp
)

ecm_add_qtwayland_client_protocol(corecomponentstest_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
    BASENAME kde-primary-output-v1
)

ecm_add_qtwayland_client_protocol(corecomponentstest_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
    BASENAME kde-output-order-v1
)

add_executable(corecomponentstest EXCLUDE_FROM_ALL ${corecomponentstest_SRCS})

target_include_directories(corecomponentstest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/plasma/extended
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/tools
)

target_link_libraries(corecomponentstest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    Plasma::Plasma
    ${LATTE_WAYLANDCLIENT_TARGET}
    Wayland::Client
)

latte_add_offscreen_test(corecomponentstest)

set(qmlsmoketest_SRCS
    qmlsmoketest.cpp
)

add_executable(qmlsmoketest EXCLUDE_FROM_ALL ${qmlsmoketest_SRCS})
add_dependencies(qmlsmoketest lattecoreplugin)

target_compile_definitions(qmlsmoketest PRIVATE
    LATTE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    LATTE_CORE_QMLDIR="${CMAKE_SOURCE_DIR}/declarativeimports/core/qmldir"
    LATTE_CORE_PLUGIN="$<TARGET_FILE:lattecoreplugin>"
    LATTE_RESTORE_ANIMATION_QML="${CMAKE_SOURCE_DIR}/declarativeimports/abilities/items/basicitem/RestoreAnimation.qml"
    LATTE_SHOW_WINDOW_ANIMATION_QML="${CMAKE_SOURCE_DIR}/plasmoid/package/contents/ui/task/animations/ShowWindowAnimation.qml"
    LATTE_PARABOLIC_ITEM_QML="${CMAKE_SOURCE_DIR}/declarativeimports/abilities/items/basicitem/ParabolicItem.qml"
    LATTE_PARABOLICAREA_QML="${CMAKE_SOURCE_DIR}/containment/package/contents/ui/applet/ParabolicArea.qml"
    LATTE_COMPACT_APPLET_QML="${CMAKE_SOURCE_DIR}/shell/package/contents/applet/CompactApplet.qml"
    LATTE_LAUNCHERS_QML="${CMAKE_SOURCE_DIR}/plasmoid/package/contents/ui/abilities/Launchers.qml"
    LATTE_PULSEAUDIO_QML="${CMAKE_SOURCE_DIR}/plasmoid/package/contents/ui/PulseAudio.qml"
    LATTE_TASKMOUSEAREA_QML="${CMAKE_SOURCE_DIR}/plasmoid/package/contents/ui/task/TaskMouseArea.qml"
    LATTE_TASKSCONFIG_QML="${CMAKE_SOURCE_DIR}/shell/package/contents/configuration/pages/TasksConfig.qml"
    LATTE_CONFIG_INTERACTION_QML="${CMAKE_SOURCE_DIR}/plasmoid/package/contents/ui/config/ConfigInteraction.qml"
    LATTE_TYPES_H="${CMAKE_SOURCE_DIR}/plasmoid/plugin/types.h"
)

target_link_libraries(qmlsmoketest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
)

latte_add_offscreen_test(qmlsmoketest)

set(sourcecontracttest_SRCS
    sourcecontracttest.cpp
)

add_executable(sourcecontracttest EXCLUDE_FROM_ALL ${sourcecontracttest_SRCS})

target_compile_definitions(sourcecontracttest PRIVATE
    LATTE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(sourcecontracttest
    Qt6::Core
    Qt6::Test
)

latte_add_offscreen_test(sourcecontracttest)

set(packagingcontracttest_SRCS
    packagingcontracttest.cpp
)

add_executable(packagingcontracttest EXCLUDE_FROM_ALL ${packagingcontracttest_SRCS})

target_compile_definitions(packagingcontracttest PRIVATE
    LATTE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(packagingcontracttest
    Qt6::Core
    Qt6::Test
)

latte_add_offscreen_test(packagingcontracttest)

set(typesunittest_SRCS
    typesunittest.cpp
    ../containment/plugin/types.cpp
    ../plasmoid/plugin/types.cpp
)

add_executable(typesunittest EXCLUDE_FROM_ALL ${typesunittest_SRCS})

target_include_directories(typesunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(typesunittest
    Qt6::Core
    Qt6::Test
)

latte_add_test(typesunittest)

find_package(PlasmaActivitiesStats REQUIRED)

set(taskpluginunittest_SRCS
    taskpluginunittest.cpp
    ../plasmoid/plugin/contextmenuactionsbackend.cpp
)

add_executable(taskpluginunittest EXCLUDE_FROM_ALL ${taskpluginunittest_SRCS})

target_include_directories(taskpluginunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../plasmoid/plugin
)

target_link_libraries(taskpluginunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::I18n
    KF6::KIOCore
    KF6::KIOFileWidgets
    KF6::KIOWidgets
    Plasma::ActivitiesStats
    Plasma::Plasma
)

latte_add_test(taskpluginunittest)

set(packageunittest_SRCS
    packageunittest.cpp
    ../app/package/lattepackage.cpp
)

add_executable(packageunittest EXCLUDE_FROM_ALL ${packageunittest_SRCS})
add_dependencies(packageunittest latte_packagestructure_indicator)

target_compile_definitions(packageunittest PRIVATE
    LATTE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(packageunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/package
)

target_link_libraries(packageunittest
    Qt6::Core
    Qt6::Test
    KF6::I18n
    KF6::Package
)

latte_add_test(packageunittest)
set_tests_properties(packageunittest PROPERTIES ENVIRONMENT "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin")

set(indicatorunittest_SRCS
    indicatorunittest.cpp
    ../app/indicator/factory.cpp
)

add_executable(indicatorunittest EXCLUDE_FROM_ALL ${indicatorunittest_SRCS})

target_include_directories(indicatorunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/indicator
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/layouts
)

target_link_libraries(indicatorunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::Archive
    KF6::CoreAddons
    KF6::I18n
    KF6::Notifications
    KF6::XmlGui
)

latte_add_offscreen_test(indicatorunittest)

set(layoutunittest_SRCS
    layoutunittest.cpp
    ../app/layout/abstractlayout.cpp
)

add_executable(layoutunittest EXCLUDE_FROM_ALL ${layoutunittest_SRCS})

target_include_directories(layoutunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/layout
)

target_link_libraries(layoutunittest
    Qt6::Core
    Qt6::Test
    KF6::ConfigCore
    Plasma::Plasma
)

latte_add_test(layoutunittest)

set(launcherunittest_SRCS
    launcherunittest.cpp
    ../app/layouts/syncedlaunchers.cpp
)

add_executable(launcherunittest EXCLUDE_FROM_ALL ${launcherunittest_SRCS})

target_include_directories(launcherunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/layouts
)

target_link_libraries(launcherunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Test
    KF6::ConfigCore
    Plasma::Plasma
)

latte_add_offscreen_test(launcherunittest)

set(genericsettingstest_SRCS
    genericsettingstest.cpp
    ../app/settings/generic/genericdialog.cpp
    ../app/settings/generic/generichandler.cpp
)

add_executable(genericsettingstest EXCLUDE_FROM_ALL ${genericsettingstest_SRCS})

target_include_directories(genericsettingstest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/generic
)

target_link_libraries(genericsettingstest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::I18n
    KF6::XmlGui
)

latte_add_offscreen_test(genericsettingstest)

set(schemecolorsunittest_SRCS
    schemecolorsunittest.cpp
    ../app/settings/detailsdialog/schemesmodel.cpp
    ../app/wm/schemecolors.cpp
)

add_executable(schemecolorsunittest EXCLUDE_FROM_ALL ${schemecolorsunittest_SRCS})

target_include_directories(schemecolorsunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/layouts
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/detailsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/wm
)

target_link_libraries(schemecolorsunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    Plasma::Plasma
)

latte_add_test(schemecolorsunittest)

set(wmunittest_SRCS
    wmunittest.cpp
    ../app/wm/tasktools.cpp
    ../app/wm/windowinfowrap.cpp
)

add_executable(wmunittest EXCLUDE_FROM_ALL ${wmunittest_SRCS})

target_include_directories(wmunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/wm
)

target_link_libraries(wmunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::Service
    KF6::WindowSystem
    Plasma::Activities
)

latte_add_offscreen_test(wmunittest)

set(toolsunittest_SRCS
    toolsunittest.cpp
    ../app/data/activitydata.cpp
    ../app/data/appletdata.cpp
    ../app/data/errordata.cpp
    ../app/data/errorinformationdata.cpp
    ../app/data/genericbasictable.cpp
    ../app/data/genericdata.cpp
    ../app/data/generictable.cpp
    ../app/data/layoutdata.cpp
    ../app/data/layoutcolordata.cpp
    ../app/data/layouticondata.cpp
    ../app/data/layoutstable.cpp
    ../app/data/preferencesdata.cpp
    ../app/data/screendata.cpp
    ../app/data/uniqueidinfo.cpp
    ../app/data/viewdata.cpp
    ../app/data/viewstable.cpp
    ../app/settings/actionsdialog/actionlistwidgetitem.cpp
    ../app/settings/exporttemplatedialog/delegates/normalcheckboxdelegate.cpp
    ../app/settings/detailsdialog/delegates/schemecmbitemdelegate.cpp
    ../app/settings/detailsdialog/schemescombobox.cpp
    ../app/settings/generic/generictools.cpp
    ../app/settings/generic/genericviewtools.cpp
    ../app/settings/generic/layoutscombobox.cpp
    ../app/settings/generic/persistentmenu.cpp
    ../app/settings/settingsdialog/delegates/activitiesdelegate.cpp
    ../app/settings/settingsdialog/delegates/backgrounddelegate.cpp
    ../app/settings/settingsdialog/delegates/checkboxdelegate.cpp
    ../app/settings/settingsdialog/delegates/layoutcmbitemdelegate.cpp
    ../app/settings/settingsdialog/delegates/layoutnamedelegate.cpp
    ../app/settings/screensdialog/delegates/checkboxdelegate.cpp
    ../app/settings/viewsdialog/delegates/custommenuitemwidget.cpp
    ../app/settings/viewsdialog/delegates/namedelegate.cpp
    ../app/settings/viewsdialog/delegates/singleoptiondelegate.cpp
    ../app/settings/viewsdialog/delegates/singletextdelegate.cpp
    ../app/tools/commontools.cpp
)

add_executable(toolsunittest EXCLUDE_FROM_ALL ${toolsunittest_SRCS})

target_include_directories(toolsunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/actionsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/exporttemplatedialog/delegates
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/detailsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/detailsdialog/delegates
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/generic
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/settingsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/settingsdialog/delegates
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/screensdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/screensdialog/delegates
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/viewsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/viewsdialog/delegates
)

target_link_libraries(toolsunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::I18n
    KF6::XmlGui
    ${LATTE_SVG_TARGET}
    Plasma::Activities
    Plasma::Plasma
)

latte_add_offscreen_test(toolsunittest)


set(knscompatunittest_SRCS
    knscompatunittest.cpp
    ../app/knscompat.cpp
)

add_executable(knscompatunittest EXCLUDE_FROM_ALL ${knscompatunittest_SRCS})

target_include_directories(knscompatunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
)

target_link_libraries(knscompatunittest
    Qt6::Core
    Qt6::Test
)

latte_add_test(knscompatunittest)

set(interfacesunittest_SRCS
    interfacesunittest.cpp
    ../app/declarativeimports/interfaces.cpp
)

add_executable(interfacesunittest EXCLUDE_FROM_ALL ${interfacesunittest_SRCS})

target_include_directories(interfacesunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/declarativeimports
)

target_link_libraries(interfacesunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Test
    Plasma::PlasmaQuick
)

latte_add_test(interfacesunittest)


set(viewpartunittest_SRCS
    viewpartunittest.cpp
    ../app/view/tasksmodel.cpp
    ../app/view/indicator/indicatorinfo.cpp
    ../app/view/indicator/indicatorresources.cpp
    indicatoruipathstub.cpp
)

add_executable(viewpartunittest EXCLUDE_FROM_ALL ${viewpartunittest_SRCS})

target_include_directories(viewpartunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/view
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/view/indicator
)

target_link_libraries(viewpartunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    ${LATTE_SVG_TARGET}
    KF6::ConfigCore
    KF6::ConfigGui
    Plasma::Plasma
    Plasma::PlasmaQuick
)

latte_add_test(viewpartunittest)

set(panelshadowunittest_SRCS
    panelshadowunittest.cpp
    ../app/view/panelshadows.cpp
)

add_executable(panelshadowunittest EXCLUDE_FROM_ALL ${panelshadowunittest_SRCS})

target_include_directories(panelshadowunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/view
)

target_link_libraries(panelshadowunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    ${LATTE_SVG_TARGET}
    KF6::WindowSystem
    Plasma::Plasma
)

latte_add_offscreen_test(panelshadowunittest)

set(contextmenuunittest_SRCS
    contextmenuunittest.cpp
    ../containmentactions/contextmenu/layoutmenuitemwidget.cpp
    ../app/settings/generic/generictools.cpp
)

add_executable(contextmenuunittest EXCLUDE_FROM_ALL ${contextmenuunittest_SRCS})

target_include_directories(contextmenuunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../containmentactions/contextmenu
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/generic
)

target_link_libraries(contextmenuunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
)

latte_add_offscreen_test(contextmenuunittest)

set(shortcutunittest_SRCS
    shortcutunittest.cpp
    ../app/shortcuts/modifiertracker.cpp
    ../app/shortcuts/shortcutstracker.cpp
    ../app/tools/commontools.cpp
)

add_executable(shortcutunittest EXCLUDE_FROM_ALL ${shortcutunittest_SRCS})

target_include_directories(shortcutunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/shortcuts
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
)

target_link_libraries(shortcutunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::GuiAddons
)

latte_add_test(shortcutunittest)

set(declarativecoreunittest_SRCS
    declarativecoreunittest.cpp
    ../declarativeimports/core/dialog.cpp
    ../declarativeimports/core/iconitem.cpp
)

add_executable(declarativecoreunittest EXCLUDE_FROM_ALL ${declarativecoreunittest_SRCS})

target_include_directories(declarativecoreunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../declarativeimports/core
    ${CMAKE_BINARY_DIR}/declarativeimports/core
)

target_link_libraries(declarativecoreunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Test
    ${LATTE_SVG_TARGET}
    KF6::GuiAddons
    KF6::IconThemes
    Plasma::Plasma
    Plasma::PlasmaQuick
)

latte_add_offscreen_test(declarativecoreunittest)

set(containmentlayoutunittest_SRCS
    containmentlayoutunittest.cpp
    ../containment/plugin/layoutmanager.cpp
)

add_executable(containmentlayoutunittest EXCLUDE_FROM_ALL ${containmentlayoutunittest_SRCS})

target_include_directories(containmentlayoutunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../containment/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_BINARY_DIR}/containment
)

target_link_libraries(containmentlayoutunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    KF6::ConfigCore
    Plasma::Plasma
    Plasma::PlasmaQuick
)

latte_add_offscreen_test(containmentlayoutunittest)


set(pluginregistrationunittest_SRCS
    pluginregistrationunittest.cpp
    ../containment/plugin/lattecontainmentplugin.cpp
    ../containment/plugin/layoutmanager.cpp
    ../containment/plugin/types.h
)

add_executable(pluginregistrationunittest EXCLUDE_FROM_ALL ${pluginregistrationunittest_SRCS})

target_include_directories(pluginregistrationunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../containment/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_BINARY_DIR}/containment
)

target_link_libraries(pluginregistrationunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    KF6::ConfigCore
    Plasma::Plasma
    Plasma::PlasmaQuick
)

latte_add_test(pluginregistrationunittest)


set(plasmaextendedunittest_SRCS
    plasmaextendedunittest.cpp
    ../app/plasma/extended/panelbackground.cpp
    panelthemeshadowstub.cpp
)

add_executable(plasmaextendedunittest EXCLUDE_FROM_ALL ${plasmaextendedunittest_SRCS})

target_include_directories(plasmaextendedunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/plasma/extended
)

target_link_libraries(plasmaextendedunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    ${LATTE_SVG_TARGET}
    Plasma::Plasma
)

latte_add_test(plasmaextendedunittest)



set(taskspluginunittest_SRCS
    taskspluginunittest.cpp
    ../plasmoid/plugin/lattetasksplugin.cpp
    ../plasmoid/plugin/contextmenuactionsbackend.cpp
    ../plasmoid/plugin/types.h
)

add_executable(taskspluginunittest EXCLUDE_FROM_ALL ${taskspluginunittest_SRCS})

target_include_directories(taskspluginunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../plasmoid/plugin
)

target_link_libraries(taskspluginunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
    KF6::KIOCore
    KF6::KIOGui
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    Plasma::ActivitiesStats
    Plasma::PlasmaQuick
)

latte_add_test(taskspluginunittest)


set(containmentactionmenuunittest_SRCS
    containmentactionmenuunittest.cpp
    ../containmentactions/contextmenu/menu.cpp
    ../containmentactions/contextmenu/layoutmenuitemwidget.cpp
    ../app/settings/generic/generictools.cpp
)

add_executable(containmentactionmenuunittest EXCLUDE_FROM_ALL ${containmentactionmenuunittest_SRCS})

target_include_directories(containmentactionmenuunittest PRIVATE
    ${CMAKE_BINARY_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/generic
    ${CMAKE_CURRENT_SOURCE_DIR}/../containmentactions/contextmenu
)

target_link_libraries(containmentactionmenuunittest
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::I18n
    KF6::XmlGui
    Plasma::Plasma
)

latte_add_offscreen_test(containmentactionmenuunittest)


set(settingsviewunittest_SRCS
    settingsviewunittest.cpp
    ../app/settings/settingsdialog/layoutstableview.cpp
    ../app/settings/viewsdialog/viewstableview.cpp
)

add_executable(settingsviewunittest EXCLUDE_FROM_ALL ${settingsviewunittest_SRCS})

target_include_directories(settingsviewunittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/settingsdialog
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/settings/viewsdialog
)

target_link_libraries(settingsviewunittest
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
    KF6::I18n
)

latte_add_offscreen_test(settingsviewunittest)

set(positionergeometrytest_SRCS positionergeometrytest.cpp)
add_executable(positionergeometrytest EXCLUDE_FROM_ALL ${positionergeometrytest_SRCS})
target_include_directories(positionergeometrytest PRIVATE ${CMAKE_BINARY_DIR}/app ${CMAKE_CURRENT_SOURCE_DIR}/../app)
target_link_libraries(positionergeometrytest Qt6::Core Qt6::Gui Qt6::Test Plasma::Plasma)
latte_add_test(positionergeometrytest)

set(parabolicmathtest_SRCS parabolicmathtest.cpp)
add_executable(parabolicmathtest EXCLUDE_FROM_ALL ${parabolicmathtest_SRCS})
target_link_libraries(parabolicmathtest Qt6::Core Qt6::Test)
latte_add_test(parabolicmathtest)

set(visibilitylogictest_SRCS visibilitylogictest.cpp)
add_executable(visibilitylogictest EXCLUDE_FROM_ALL ${visibilitylogictest_SRCS})
target_link_libraries(visibilitylogictest Qt6::Core Qt6::Gui Qt6::Test Plasma::Plasma)
latte_add_test(visibilitylogictest)

set(storagelogictest_SRCS storagelogictest.cpp)
add_executable(storagelogictest EXCLUDE_FROM_ALL ${storagelogictest_SRCS})
target_include_directories(storagelogictest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../app ${CMAKE_CURRENT_SOURCE_DIR}/../app/data)
target_link_libraries(storagelogictest Qt6::Core Qt6::Test KF6::ConfigCore)
latte_add_test(storagelogictest)

set(synchronizerlogictest_SRCS synchronizerlogictest.cpp)
add_executable(synchronizerlogictest EXCLUDE_FROM_ALL ${synchronizerlogictest_SRCS})
target_link_libraries(synchronizerlogictest Qt6::Core Qt6::Test)
latte_add_test(synchronizerlogictest)

set(effectslogictest_SRCS effectslogictest.cpp)
add_executable(effectslogictest EXCLUDE_FROM_ALL ${effectslogictest_SRCS})
target_include_directories(effectslogictest PRIVATE ${CMAKE_BINARY_DIR}/app ${CMAKE_CURRENT_SOURCE_DIR}/../app)
target_link_libraries(effectslogictest Qt6::Core Qt6::Gui Qt6::Test Plasma::Plasma)
latte_add_test(effectslogictest)

add_custom_target(latte-autotests
    DEPENDS ${latte_autotest_targets}
)
