
set(kio_recentdocuments_PART_SRCS
   recentdocuments.cpp)

add_library(kio_recentdocuments MODULE ${kio_recentdocuments_PART_SRCS})

target_link_libraries(kio_recentdocuments KF5::KIOCore)

install(TARGETS kio_recentdocuments  DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(kded_recentdocumentsnotifier_PART_SRCS  recentdocumentsnotifier.cpp )

add_library(kded_recentdocumentsnotifier MODULE  ${kded_recentdocumentsnotifier_PART_SRCS})

target_link_libraries(kded_recentdocumentsnotifier  KF5::KIOCore)

install(TARGETS kded_recentdocumentsnotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES recentdocumentsnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES recentdocuments.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
