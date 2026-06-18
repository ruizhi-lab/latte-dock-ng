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

add_test(NAME dataunittest COMMAND dataunittest)

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

add_test(NAME modelunittest COMMAND modelunittest)

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

add_test(NAME coreunittest COMMAND coreunittest)
set_tests_properties(coreunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME corecomponentstest COMMAND corecomponentstest)
set_tests_properties(corecomponentstest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

set(qmlsmoketest_SRCS
    qmlsmoketest.cpp
)

add_executable(qmlsmoketest EXCLUDE_FROM_ALL ${qmlsmoketest_SRCS})
add_dependencies(qmlsmoketest lattecoreplugin)

target_compile_definitions(qmlsmoketest PRIVATE
    LATTE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    LATTE_CORE_QMLDIR="${CMAKE_SOURCE_DIR}/declarativeimports/core/qmldir"
    LATTE_CORE_PLUGIN="$<TARGET_FILE:lattecoreplugin>"
)

target_link_libraries(qmlsmoketest
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
)

add_test(NAME qmlsmoketest COMMAND qmlsmoketest)
set_tests_properties(qmlsmoketest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME sourcecontracttest COMMAND sourcecontracttest)
set_tests_properties(sourcecontracttest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME packagingcontracttest COMMAND packagingcontracttest)
set_tests_properties(packagingcontracttest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME typesunittest COMMAND typesunittest)

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

add_test(NAME taskpluginunittest COMMAND taskpluginunittest)

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

add_test(NAME packageunittest COMMAND packageunittest)
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

add_test(NAME indicatorunittest COMMAND indicatorunittest)
set_tests_properties(indicatorunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME layoutunittest COMMAND layoutunittest)

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

add_test(NAME launcherunittest COMMAND launcherunittest)
set_tests_properties(launcherunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME genericsettingstest COMMAND genericsettingstest)
set_tests_properties(genericsettingstest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME schemecolorsunittest COMMAND schemecolorsunittest)

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

add_test(NAME wmunittest COMMAND wmunittest)
set_tests_properties(wmunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME toolsunittest COMMAND toolsunittest)
set_tests_properties(toolsunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")


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

add_test(NAME knscompatunittest COMMAND knscompatunittest)

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

add_test(NAME interfacesunittest COMMAND interfacesunittest)


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

add_test(NAME viewpartunittest COMMAND viewpartunittest)

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

add_test(NAME panelshadowunittest COMMAND panelshadowunittest)
set_tests_properties(panelshadowunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME contextmenuunittest COMMAND contextmenuunittest)
set_tests_properties(contextmenuunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME shortcutunittest COMMAND shortcutunittest)

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

add_test(NAME declarativecoreunittest COMMAND declarativecoreunittest)
set_tests_properties(declarativecoreunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

add_test(NAME containmentlayoutunittest COMMAND containmentlayoutunittest)
set_tests_properties(containmentlayoutunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")


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

add_test(NAME pluginregistrationunittest COMMAND pluginregistrationunittest)


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

add_test(NAME plasmaextendedunittest COMMAND plasmaextendedunittest)



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

add_test(NAME taskspluginunittest COMMAND taskspluginunittest)


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

add_test(NAME containmentactionmenuunittest COMMAND containmentactionmenuunittest)
set_tests_properties(containmentactionmenuunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")


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

add_test(NAME settingsviewunittest COMMAND settingsviewunittest)
set_tests_properties(settingsviewunittest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_custom_target(latte-autotests
    DEPENDS
        dataunittest
        modelunittest
        coreunittest
        corecomponentstest
        qmlsmoketest
        sourcecontracttest
        packagingcontracttest
        typesunittest
        taskpluginunittest
        packageunittest
        indicatorunittest
        layoutunittest
        launcherunittest
        genericsettingstest
        schemecolorsunittest
        wmunittest
        toolsunittest
        knscompatunittest
        interfacesunittest
        viewpartunittest
        panelshadowunittest
        contextmenuunittest
        shortcutunittest
        declarativecoreunittest
        containmentlayoutunittest
        pluginregistrationunittest
        plasmaextendedunittest
        taskspluginunittest
        containmentactionmenuunittest
        settingsviewunittest
)
