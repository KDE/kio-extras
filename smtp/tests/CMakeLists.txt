
########### next target ###############

set(test_responseparser_SRCS test_responseparser.cc )


kde4_add_unit_test(test_responseparser ${test_responseparser_SRCS})

target_link_libraries(test_responseparser  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

########### next target ###############

set(test_headergeneration_SRCS test_headergeneration.cc )


kde4_add_test_executable(test_headergeneration ${test_headergeneration_SRCS})

target_link_libraries(test_headergeneration  ${KDE4_KDECORE_LIBS} )


########### next target ###############
if(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)
set(test_commands_SRCS test_commands.cc )


kde4_add_test_executable(test_commands ${test_commands_SRCS})

target_link_libraries(test_commands  ${KDE4_KDECORE_LIBS} )

endif(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

########### next target ###############
set(interactivesmtpserver_SRCS interactivesmtpserver.cc )


kde4_add_test_executable(interactivesmtpserver ${interactivesmtpserver_SRCS})

target_link_libraries(interactivesmtpserver  ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} )


