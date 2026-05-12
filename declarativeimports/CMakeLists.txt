add_subdirectory(core)
install(DIRECTORY abilities/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/abilities)
install(DIRECTORY components/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/components)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(DIRECTORY abilities/ DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/abilities)
    install(DIRECTORY components/ DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/components)
endif()

