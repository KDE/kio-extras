



########### next target ###############

set(kio_about_PART_SRCS kio_about.cpp )


add_library(kio_about ${kio_about_PART_SRCS})


target_link_libraries(kio_about  KF5::KIOCore Qt5::Network)

install(TARGETS kio_about  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES about.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
