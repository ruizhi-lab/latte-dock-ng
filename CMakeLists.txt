cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(lattedock)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VERSION 0.10.77)
set(AUTHOR "Michail Vourlakos, Smith Ar")
set(EMAIL "mvourlakos@gmail.com, audoban@openmailbox.org")
set(WEBSITE "https://userbase.kde.org/LatteDock")
set(BUG_ADDRESS "https://bugs.kde.org/enter_bug.cgi?product=lattedock")
set(FAQS "https://userbase.kde.org/LatteDock/FAQ")

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")
set(PLASMA_MIN_VERSION "6.6.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0.0")

set(KF6_LOCALE_PREFIX "")
set(QT_MAJOR_VERSION 6)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(CheckIncludeFiles)
include(CMakePackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEPackageAppTemplates)
include(WriteBasicConfigVersionFile)

# KDEInstallDirs6 dropped KSERVICETYPESDIR; keep a deterministic install path for Latte service types.
set(LATTE_KSERVICETYPES_INSTALL_DIR "${KDE_INSTALL_DATADIR}/kservicetypes6")
if(DEFINED KDE_INSTALL_KSERVICETYPESDIR AND NOT KDE_INSTALL_KSERVICETYPESDIR STREQUAL "")
    set(LATTE_KSERVICETYPES_INSTALL_DIR "${KDE_INSTALL_KSERVICETYPESDIR}")
endif()


find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED NO_MODULE COMPONENTS Core DBus Gui Qml Quick Widgets)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Archive Config CoreAddons GuiAddons Crash DBusAddons Declarative GlobalAccel
    I18n IconThemes KIO NewStuff Notifications Package Svg WindowSystem XmlGui)

find_package(Plasma ${PLASMA_MIN_VERSION} REQUIRED)
find_package(PlasmaQuick ${PLASMA_MIN_VERSION} REQUIRED)
find_package(PlasmaActivities ${PLASMA_MIN_VERSION} REQUIRED)
find_package(KWayland ${PLASMA_MIN_VERSION} REQUIRED)
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

# Distros may expose KDEclarative targets with different names/casing.
set(LATTE_DECLARATIVE_TARGET "")
set(LATTE_QUICKADDONS_TARGET "")
set(LATTE_NEWSTUFF_TARGET "")
set(LATTE_WAYLANDCLIENT_TARGET "")
set(LATTE_CONFIGQML_TARGET "")
set(LATTE_SVG_TARGET "")

get_property(_latte_imported_targets DIRECTORY PROPERTY IMPORTED_TARGETS)

foreach(_candidate KF6::Declarative KF6::KDeclarative)
    if(TARGET ${_candidate})
        set(LATTE_DECLARATIVE_TARGET ${_candidate})
        break()
    endif()
endforeach()

foreach(_candidate KF6::QuickAddons KF6::KQuickAddons)
    if(TARGET ${_candidate})
        set(LATTE_QUICKADDONS_TARGET ${_candidate})
        break()
    endif()
endforeach()

foreach(_candidate KF6::NewStuffWidgets KF6::KNewStuffWidgets KF6::NewStuff KF6::KNewStuff)
    if(TARGET ${_candidate})
        set(LATTE_NEWSTUFF_TARGET ${_candidate})
        break()
    endif()
endforeach()

foreach(_candidate KWayland::Client KF6::WaylandClient KF6::KWaylandClient KF5::WaylandClient)
    if(TARGET ${_candidate})
        set(LATTE_WAYLANDCLIENT_TARGET ${_candidate})
        break()
    endif()
endforeach()

foreach(_candidate KF6::ConfigQml KF6::KConfigQml)
    if(TARGET ${_candidate})
        set(LATTE_CONFIGQML_TARGET ${_candidate})
        break()
    endif()
endforeach()

foreach(_candidate KF6::Svg KF6::KSvg KF6::KSvgCore)
    if(TARGET ${_candidate})
        set(LATTE_SVG_TARGET ${_candidate})
        break()
    endif()
endforeach()

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "kdeclarative")
            set(LATTE_DECLARATIVE_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "quickaddons")
            set(LATTE_QUICKADDONS_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "newstuff" AND _target_lc MATCHES "widget")
            set(LATTE_NEWSTUFF_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "newstuff")
            set(LATTE_NEWSTUFF_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_WAYLANDCLIENT_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "waylandclient" OR _target_lc MATCHES "kwayland::client")
            set(LATTE_WAYLANDCLIENT_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_CONFIGQML_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "configqml")
            set(LATTE_CONFIGQML_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_SVG_TARGET}" STREQUAL "")
    foreach(_target ${_latte_imported_targets})
        string(TOLOWER "${_target}" _target_lc)
        if(_target_lc MATCHES "::svg" OR _target_lc MATCHES "ksvg")
            set(LATTE_SVG_TARGET ${_target})
            break()
        endif()
    endforeach()
endif()

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    foreach(_var KF6Declarative_LIBRARIES KDeclarative_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            string(TOLOWER "${${_var}}" _candidate_lc)
            if(NOT _candidate_lc MATCHES "_location$")
                set(LATTE_DECLARATIVE_TARGET "${${_var}}")
                break()
            endif()
        endif()
    endforeach()
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    foreach(_var QuickAddons_LIBRARIES KF6QuickAddons_LIBRARIES KQuickAddons_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            set(LATTE_QUICKADDONS_TARGET "${${_var}}")
            break()
        endif()
    endforeach()
endif()

if("${LATTE_NEWSTUFF_TARGET}" STREQUAL "")
    foreach(_var
            NewStuffWidgets_LIBRARIES KF6NewStuffWidgets_LIBRARIES KNewStuffWidgets_LIBRARIES
            NewStuff_LIBRARIES KF6NewStuff_LIBRARIES KNewStuff_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            set(LATTE_NEWSTUFF_TARGET "${${_var}}")
            break()
        endif()
    endforeach()
endif()

if("${LATTE_WAYLANDCLIENT_TARGET}" STREQUAL "")
    foreach(_var
            KWaylandClient_LIBRARIES KF6WaylandClient_LIBRARIES KF5WaylandClient_LIBRARIES
            WaylandClient_LIBRARIES KWayland_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            set(LATTE_WAYLANDCLIENT_TARGET "${${_var}}")
            break()
        endif()
    endforeach()
endif()

if("${LATTE_CONFIGQML_TARGET}" STREQUAL "")
    foreach(_var ConfigQml_LIBRARIES KF6ConfigQml_LIBRARIES KConfigQml_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            set(LATTE_CONFIGQML_TARGET "${${_var}}")
            break()
        endif()
    endforeach()
endif()

if("${LATTE_SVG_TARGET}" STREQUAL "")
    foreach(_var Svg_LIBRARIES KF6Svg_LIBRARIES KSvg_LIBRARIES)
        if(DEFINED ${_var} AND NOT "${${_var}}" STREQUAL "")
            set(LATTE_SVG_TARGET "${${_var}}")
            break()
        endif()
    endforeach()
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    set(LATTE_QUICKADDONS_TARGET ${LATTE_DECLARATIVE_TARGET})
endif()

if("${LATTE_QUICKADDONS_TARGET}" STREQUAL "")
    message(WARNING
        "Could not resolve a KQuickAddons CMake link target. "
        "Proceeding without explicit KQuickAddons link target.")
endif()

if("${LATTE_DECLARATIVE_TARGET}" STREQUAL "")
    message(WARNING
        "Could not resolve a KDEclarative CMake link target. "
        "Proceeding without explicit KDeclarative link target.")
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
ecm_find_qmlmodule(Qt5Compat.GraphicalEffects 1.0)
ecm_find_qmlmodule(org.kde.kquickcontrolsaddons 2.0)
ecm_find_qmlmodule(org.kde.plasma.core 2.0)
ecm_find_qmlmodule(org.kde.plasma.components 2.0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/config-latte.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/app/config-latte.h)

#hide warnings
string(REPLACE "-Wall" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "-Wdeprecated-declarations" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "-Wreorder" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "-Wunused-variable" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "-Wunused-parameter" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

#add format security check
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Werror=format-security")
# KDECompilerSettings may enforce stricter Qt macros depending on distro patching.
# Keep Latte sources buildable on Plasma 6.6+ by explicitly dropping those macros here.
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
add_subdirectory(declarativeimports)
add_subdirectory(indicators)
add_subdirectory(app)
add_subdirectory(containmentactions)
add_subdirectory(containment)
add_subdirectory(icons)
add_subdirectory(plasmoid)
add_subdirectory(shell)

ki18n_install(po)
