


########### next target ###############

set(kded_remotedirnotify_PART_SRCS remotedirnotify.cpp remotedirnotifymodule.cpp )


add_library(kded_remotedirnotify MODULE ${kded_remotedirnotify_PART_SRCS})


target_link_libraries(kded_remotedirnotify  KF5::KIOCore)

install(TARGETS kded_remotedirnotify  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES remotedirnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

