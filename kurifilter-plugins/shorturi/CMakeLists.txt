add_library(kshorturifilter MODULE kshorturifilter.cpp)

target_link_libraries(kshorturifilter KF5::I18n KF5::KIOWidgets KF5::KDE4Support)

install(TARGETS kshorturifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kshorturifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kshorturifilterrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

