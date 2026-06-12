# Compatibility QML modules
#
# org.kde.latte.compat.taskmanager provides a minimal fallback for the
# org.kde.plasma.private.taskmanager private module that some Plasma 6
# distributions omit. It lives in latte's own namespace so install/uninstall
# never touches system Plasma directories.

set(_compat_taskmanager_relpath "org/kde/latte/compat/taskmanager")
set(_compat_taskmanager_dir "${KDE_INSTALL_FULL_QMLDIR}/${_compat_taskmanager_relpath}")

message(STATUS "Installing compat taskmanager QML module to ${_compat_taskmanager_dir}")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_compat_taskmanager_relpath}/"
        DESTINATION "${KDE_INSTALL_QMLDIR}/${_compat_taskmanager_relpath}")
