
project(kioslave-nntp)



########### next target ###############

set(kio_nntp_PART_SRCS nntp.cpp )

kde4_automoc(${kio_nntp_PART_SRCS})

kde4_add_plugin(kio_nntp ${kio_nntp_PART_SRCS})


target_link_libraries(kio_nntp  ${KDE4_KIO_LIBS}  )

install(TARGETS kio_nntp  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES nntp.protocol nntps.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

