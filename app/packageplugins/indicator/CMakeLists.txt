kcoreaddons_add_plugin(latte_packagestructure_indicator SOURCES indicatorpackage.cpp INSTALL_NAMESPACE kpackage/packagestructure)
set_target_properties(latte_packagestructure_indicator PROPERTIES OUTPUT_NAME latte_indicator)

target_link_libraries(latte_packagestructure_indicator
   ${LATTE_DECLARATIVE_TARGET}
   KF6::I18n
   KF6::Package
)

install(FILES latte-indicator.desktop DESTINATION ${LATTE_KSERVICETYPES_INSTALL_DIR})
