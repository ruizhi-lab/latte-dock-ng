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

set(qmlsmoketest_SRCS
    qmlsmoketest.cpp
)

add_executable(qmlsmoketest EXCLUDE_FROM_ALL ${qmlsmoketest_SRCS})
add_dependencies(qmlsmoketest lattecoreplugin)

target_compile_definitions(qmlsmoketest PRIVATE
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
