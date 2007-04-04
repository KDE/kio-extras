project(kioslave-sftp)

add_definitions (-DQT3_SUPPORT)

########### next target ###############
if(KDE4_BUILD_TESTS)
set(ksshprocesstest_SRCS ksshprocesstest.cpp ksshprocess.cpp process.cpp atomicio.cpp )

kde4_automoc(${ksshprocesstest_SRCS})

kde4_add_executable(ksshprocesstest ${ksshprocesstest_SRCS})

target_link_libraries(ksshprocesstest  ${KDE4_KDECORE_LIBS} ${KDE4_KDESU_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(kio_sftp_PART_SRCS 
   process.cpp 
   atomicio.cpp 
   kio_sftp.cpp 
   sftpfileattr.cpp 
   ksshprocess.cpp )

kde4_automoc(${kio_sftp_PART_SRCS})

kde4_add_plugin(kio_sftp ${kio_sftp_PART_SRCS})


target_link_libraries(kio_sftp  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDESU_LIBS} )

install(TARGETS kio_sftp  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

