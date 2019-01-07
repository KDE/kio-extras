set(kio_about_PART_SRCS kio_about.cpp )

add_library(kio_about MODULE ${kio_about_PART_SRCS})

target_link_libraries(kio_about  KF5::KIOCore Qt5::Network)
set_target_properties(kio_about PROPERTIES OUTPUT_NAME "about")

install(TARGETS kio_about  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio )
install( FILES about.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
