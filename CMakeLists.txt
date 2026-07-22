cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(lattedock)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED VERSION)
    set(VERSION 1.2.30)
endif()
set(AUTHOR "Ruizhi Zhong")
set(EMAIL "ruizhi.zhong88@gmail.com")
set(WEBSITE "https://github.com/ruizhi-lab/latte-dock-ng")
set(BUG_ADDRESS "https://github.com/ruizhi-lab/latte-dock-ng/issues")
set(FAQS "https://github.com/ruizhi-lab/latte-dock-ng#readme")

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")
set(PLASMA_MIN_VERSION "6.5.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0.0")

# Accepted from install.sh for compatibility with KDE-style build knobs.
# Latte Dock NG does not currently auto-fetch translations at configure time.
option(KDE_L10N_AUTO_TRANSLATIONS "Compatibility flag for external install scripts." OFF)
set(KDE_L10N_BRANCH "" CACHE STRING "Compatibility translation branch name from external scripts.")
option(LATTE_INSTALL_USER_KICKERACTION_EXECUTABLE "Install user-local Plasma kicker action as trusted executable desktop file." OFF)

set(KF6_LOCALE_PREFIX "")
set(QT_MAJOR_VERSION 6)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED NO_MODULE COMPONENTS CoreTools)

include(KDEInstallDirs)
include(CheckIncludeFiles)
include(CMakePackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEPackageAppTemplates)
include(LatteCompilerWarnings)
include(LatteTargetResolution)
include(WriteBasicConfigVersionFile)
include(CTest)

# Some Plasma 6 environments include both lib/qml and lib/qt6/qml in the QML
# import path. Install Latte QML modules to both locations when they differ so
# stale user-level debug installs cannot shadow the freshly built files.
set(LATTE_QT6_COMPAT_QMLDIR "${KDE_INSTALL_LIBDIR}/qt6/qml")
if("${KDE_INSTALL_QMLDIR}" STREQUAL "${LATTE_QT6_COMPAT_QMLDIR}")
    set(LATTE_QT6_COMPAT_QMLDIR "")
endif()

# KDEInstallDirs6 dropped KSERVICETYPESDIR; keep a deterministic install path for Latte service types.
set(LATTE_KSERVICETYPES_INSTALL_DIR "${KDE_INSTALL_DATADIR}/kservicetypes6")
if(DEFINED KDE_INSTALL_KSERVICETYPESDIR AND NOT KDE_INSTALL_KSERVICETYPESDIR STREQUAL "")
    set(LATTE_KSERVICETYPES_INSTALL_DIR "${KDE_INSTALL_KSERVICETYPESDIR}")
endif()

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED NO_MODULE COMPONENTS Core DBus Gui Qml Quick Widgets)
if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED NO_MODULE COMPONENTS Test)
endif()

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Archive Config CoreAddons GuiAddons Crash DBusAddons Declarative GlobalAccel
    I18n IconThemes KIO NewStuff Notifications Package Svg WindowSystem XmlGui)

find_package(Plasma ${PLASMA_MIN_VERSION} REQUIRED)
find_package(PlasmaQuick ${PLASMA_MIN_VERSION} REQUIRED)
find_package(PlasmaActivities ${PLASMA_MIN_VERSION} REQUIRED)
find_package(KWayland ${KF6_MIN_VERSION} REQUIRED)
find_package(LayerShellQt REQUIRED)

# Detect whether LayerShellQt::Window::setScreen(QScreen*) is available.
# It was added in LayerShellQt 6.6 and later removed upstream.
try_compile(
    LATTE_LAYERSHELL_HAS_SET_SCREEN
    SOURCES ${CMAKE_SOURCE_DIR}/cmake/CheckLayerShellSetScreen.cpp
    LINK_LIBRARIES LayerShellQt::Interface Qt6::Core Qt6::Gui
)
if(LATTE_LAYERSHELL_HAS_SET_SCREEN)
    add_definitions(-DLATTE_LAYERSHELL_HAS_SET_SCREEN)
    message(STATUS "LayerShellQt::Window::setScreen detected — enabled")
else()
    message(STATUS "LayerShellQt::Window::setScreen not available — using QWindow::setScreen only")
endif()
find_package(KF6Declarative ${KF6_MIN_VERSION} REQUIRED CONFIG)

find_package(LibTaskManager ${PLASMA_MIN_VERSION} REQUIRED)
add_definitions(-DPLASMA_WORKSPACE_VERSION="${LibTaskManager_VERSION}")


find_package(QtWaylandScanner REQUIRED)
find_package(Qt${QT_MAJOR_VERSION}WaylandClient REQUIRED)
find_package(PlasmaWaylandProtocols 1.6 REQUIRED)
find_package(Wayland REQUIRED COMPONENTS Client)

message(STATUS "KF6 VERSION : ${KF6_VERSION}")
message(STATUS "LibTaskManager VERSION : ${LibTaskManager_VERSION}")

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/compat)

# Mageia KF6 headers live under /usr/include/KF6/<Module>/ but the cmake
# config files export /usr/include/KF6/<Module> as INTERFACE_INCLUDE_DIRECTORIES,
# so #include <KIconThemes/KIconLoader> (and similar KF6-style includes) fail.
# Add /usr/include/KF6 when it exists to restore the expected include layout.
if(EXISTS "/usr/include/KF6")
    include_directories("/usr/include/KF6")
endif()

# Distros may expose KDEclarative targets with different names/casing.
set(LATTE_DECLARATIVE_TARGET "")
set(LATTE_QUICKADDONS_TARGET "")
set(LATTE_NEWSTUFF_TARGET "")
set(LATTE_WAYLANDCLIENT_TARGET "")
set(LATTE_CONFIGQML_TARGET "")
set(LATTE_SVG_TARGET "")

get_property(_latte_imported_targets DIRECTORY PROPERTY IMPORTED_TARGETS)

latte_resolve_target_from_candidates(LATTE_DECLARATIVE_TARGET KF6::Declarative KF6::KDeclarative)
latte_resolve_target_from_candidates(LATTE_QUICKADDONS_TARGET KF6::QuickAddons KF6::KQuickAddons)
latte_resolve_target_from_candidates(LATTE_NEWSTUFF_TARGET KF6::NewStuffWidgets KF6::KNewStuffWidgets KF6::NewStuff KF6::KNewStuff)
latte_resolve_target_from_candidates(LATTE_WAYLANDCLIENT_TARGET KWayland::Client KF6::WaylandClient KF6::KWaylandClient)
latte_resolve_target_from_candidates(LATTE_CONFIGQML_TARGET KF6::ConfigQml KF6::KConfigQml)
latte_resolve_target_from_candidates(LATTE_SVG_TARGET KF6::Svg KF6::KSvg KF6::KSvgCore)

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_DECLARATIVE_TARGET "kdeclarative")
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_QUICKADDONS_TARGET "quickaddons")
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_NEWSTUFF_TARGET "newstuff" "widget")
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_NEWSTUFF_TARGET "newstuff")
endif()

if("${LATTE_WAYLANDCLIENT_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_WAYLANDCLIENT_TARGET "waylandclient|kwayland::client")
endif()

if("${LATTE_CONFIGQML_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_CONFIGQML_TARGET "configqml")
endif()

if("${LATTE_SVG_TARGET}" STREQUAL "")
    latte_resolve_imported_target(LATTE_SVG_TARGET "::svg|ksvg")
endif()

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_DECLARATIVE_TARGET KF6Declarative_LIBRARIES KDeclarative_LIBRARIES)
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_QUICKADDONS_TARGET QuickAddons_LIBRARIES KF6QuickAddons_LIBRARIES KQuickAddons_LIBRARIES)
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_NEWSTUFF_TARGET
        NewStuffWidgets_LIBRARIES KF6NewStuffWidgets_LIBRARIES KNewStuffWidgets_LIBRARIES
        NewStuff_LIBRARIES KF6NewStuff_LIBRARIES KNewStuff_LIBRARIES)
endif()

if("${LATTE_WAYLANDCLIENT_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_WAYLANDCLIENT_TARGET
        KWaylandClient_LIBRARIES KF6WaylandClient_LIBRARIES
        WaylandClient_LIBRARIES KWayland_LIBRARIES)
endif()

if("${LATTE_CONFIGQML_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_CONFIGQML_TARGET ConfigQml_LIBRARIES KF6ConfigQml_LIBRARIES KConfigQml_LIBRARIES)
endif()

if("${LATTE_SVG_TARGET}" STREQUAL "")
    latte_resolve_library_variable(LATTE_SVG_TARGET Svg_LIBRARIES KF6Svg_LIBRARIES KSvg_LIBRARIES)
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    set(LATTE_QUICKADDONS_TARGET ${LATTE_DECLARATIVE_TARGET})
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    message(STATUS "No explicit KQuickAddons link target is exported by this KF6 build.")
endif()

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    message(STATUS "No explicit KDeclarative link target is exported by this KF6 build.")
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    message(FATAL_ERROR
        "Could not resolve a KNewStuff CMake link target. "
        "Imported targets in this directory: ${_latte_imported_targets}")
endif()

if("${LATTE_WAYLANDCLIENT_TARGET}" STREQUAL "")
    message(FATAL_ERROR
        "Could not resolve a KWayland client CMake link target. "
        "Imported targets in this directory: ${_latte_imported_targets}")
endif()

if("${LATTE_CONFIGQML_TARGET}" STREQUAL "")
    message(WARNING
        "Could not resolve a KConfigQml CMake link target. "
        "Proceeding without explicit KConfigQml link target.")
endif()

if("${LATTE_SVG_TARGET}" STREQUAL "")
    message(WARNING
        "Could not resolve a KSvg CMake link target. "
        "Proceeding without explicit KSvg link target.")
endif()

message(STATUS "Resolved KDeclarative target: ${LATTE_DECLARATIVE_TARGET}")
message(STATUS "Resolved QuickAddons target: ${LATTE_QUICKADDONS_TARGET}")
message(STATUS "Resolved NewStuff target: ${LATTE_NEWSTUFF_TARGET}")
message(STATUS "Resolved KWayland client target: ${LATTE_WAYLANDCLIENT_TARGET}")
message(STATUS "Resolved KConfigQml target: ${LATTE_CONFIGQML_TARGET}")
message(STATUS "Resolved KSvg target: ${LATTE_SVG_TARGET}")


include(ECMFindQmlModule)
ecm_find_qmlmodule(QtQuick 2.7)
ecm_find_qmlmodule(QtQuick.Layouts 1.3)
ecm_find_qmlmodule(QtQuick.Controls 2.15)
ecm_find_qmlmodule(org.kde.kquickcontrolsaddons 2.0)
ecm_find_qmlmodule(org.kde.plasma.core 2.0)
ecm_find_qmlmodule(org.kde.plasma.components 3.0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/config-latte.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/app/config-latte.h)

latte_apply_relaxed_warning_flags()

#add format security check
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Werror=format-security")
# KDECompilerSettings may enforce stricter Qt macros depending on distro patching.
# Keep Latte sources buildable on Plasma 6.5+ by explicitly dropping those macros here.
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_RESTRICTED_CAST_FROM_ASCII -DQT_NO_KEYWORDS)
add_compile_options(-UQT_NO_CAST_FROM_ASCII -UQT_RESTRICTED_CAST_FROM_ASCII)

message(STATUS "COMPILER FLAGS : ${CMAKE_CXX_FLAGS}")

# Share Same Core Types between declarativeimports and app
set(CORETYPESHEADER "LIBCORETYPES_H")
configure_file(declarativeimports/coretypes.h.in declarativeimports/core/types.h)
set(CORETYPESHEADER "APPCORETYPES_H")
configure_file(declarativeimports/coretypes.h.in app/coretypes.h)
set(CORETYPESHEADER "LATTEGENERALCONTAINMENTTYPES_H")
configure_file(declarativeimports/coretypes.h.in containment/plugin/lattetypes.h)

# Share App::Settings::GenericTools to containment actions
configure_file(app/settings/generic/generictools.h containmentactions/contextmenu/generictools.h)
configure_file(app/settings/generic/generictools.cpp containmentactions/contextmenu/generictools.cpp)
configure_file(app/data/contextmenudata.h containmentactions/contextmenu/contextmenudata.h)

# subdirectories
add_subdirectory(compat/qml)
add_subdirectory(declarativeimports)
add_subdirectory(indicators)
add_subdirectory(app)
add_subdirectory(containmentactions)
add_subdirectory(containment)
add_subdirectory(icons)
add_subdirectory(separator)
add_subdirectory(plasmoid)
add_subdirectory(shell)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ki18n_install(po)

include(LattePackaging)
