
project(kioslave-smtp)

add_definitions (-DQT3_SUPPORT)

set(smtp_optional_includes)
set(smtp_optional_libs)

macro_optional_find_package(Sasl2)

if (SASL2_FOUND)
   set(smtp_optional_includes ${smtp_optional_includes} ${SASL2_INCLUDE_DIR})
   set(smtp_optional_libs     ${smtp_optional_libs}     ${SASL2_LIBRARIES})
   set(HAVE_LIBSASL2 1)
else (SASL2_FOUND)
   set(HAVE_LIBSASL2 0)
endif (SASL2_FOUND)

configure_file(smtp-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/smtp-config.h )

include_directories(  ${smtp_optional_includes}  )


########### next target ###############

set(kio_smtp_PART_SRCS
   smtp.cc
   request.cc
   response.cc
   capabilities.cc
   command.cc
   transactionstate.cc )

kde4_automoc(${kio_smtp_PART_SRCS})

kde4_add_plugin(kio_smtp ${kio_smtp_PART_SRCS})


target_link_libraries(kio_smtp  ${KDE4_KIO_LIBS} ${smtp_optional_libs})

install(TARGETS kio_smtp  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(test_responseparser_SRCS test_responseparser.cc )

kde4_automoc(${test_responseparser_SRCS})

kde4_add_unit_test(test_responseparser ${test_responseparser_SRCS})

target_link_libraries(test_responseparser  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

########### next target ###############

set(test_headergeneration_SRCS test_headergeneration.cc )

kde4_automoc(${test_headergeneration_SRCS})

kde4_add_test_executable(test_headergeneration ${test_headergeneration_SRCS})

target_link_libraries(test_headergeneration  ${KDE4_KDECORE_LIBS} )


########### next target ###############
if(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)
set(test_commands_SRCS test_commands.cc )

kde4_automoc(${test_commands_SRCS})

kde4_add_executable(test_commands ${test_commands_SRCS})

target_link_libraries(test_commands  ${KDE4_KDECORE_LIBS} )

endif(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

########### next target ###############
set(interactivesmtpserver_SRCS interactivesmtpserver.cc )

kde4_automoc(${interactivesmtpserver_SRCS})

kde4_add_test_executable(interactivesmtpserver ${interactivesmtpserver_SRCS})

target_link_libraries(interactivesmtpserver  ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} )

########### install files ###############

install( FILES smtp.protocol smtps.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

