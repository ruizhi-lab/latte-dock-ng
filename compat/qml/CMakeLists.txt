# Compatibility QML fallbacks
#
# Some upstream private QML modules (e.g. org.kde.plasma.private.taskmanager)
# may not ship in every Plasma 6 packaging. Latte's tasks plasmoid imports
# them, so install our minimal fallback when the target path doesn't already
# carry a real module. A marker file lets us recognize our own fallback on
# subsequent reconfigures and overwrite it cleanly without clobbering a real
# upstream module if one is later installed.

set(_compat_taskmanager_relpath "org/kde/plasma/private/taskmanager")
set(_compat_taskmanager_dir "${KDE_INSTALL_FULL_QMLDIR}/${_compat_taskmanager_relpath}")
set(_compat_taskmanager_marker "${_compat_taskmanager_dir}/.latte-fallback-module")
set(_compat_taskmanager_qmldir "${_compat_taskmanager_dir}/qmldir")

if(NOT EXISTS "${_compat_taskmanager_qmldir}" OR EXISTS "${_compat_taskmanager_marker}")
    message(STATUS "Installing fallback org.kde.plasma.private.taskmanager QML module to ${_compat_taskmanager_dir}")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_compat_taskmanager_relpath}/"
            DESTINATION "${KDE_INSTALL_QMLDIR}/${_compat_taskmanager_relpath}")

    install(CODE "
        file(WRITE \"\$ENV{DESTDIR}${_compat_taskmanager_marker}\"
             \"latte-dock-ng fallback module\\n\")
    ")
else()
    message(STATUS "System org.kde.plasma.private.taskmanager already present at ${_compat_taskmanager_dir}; skipping fallback install")
endif()
