
project(kioslave-man)

add_definitions (-DQT3_SUPPORT)


########### next target ###############

set(kio_man_test_SRCS kio_man_test.cpp man2html.cpp kio_man.cpp )

kde4_automoc(${kio_man_test_SRCS})


kde4_add_test_executable(kio_man_test ${kio_man_test_SRCS})
set_target_properties(kio_man_test PROPERTIES COMPILE_FLAGS "-DSIMPLE_MAN2HTML -DKIO_MAN_TEST")
target_link_libraries(kio_man_test  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})


########### next target ###############

set(man2html_SRCS man2html.cpp )

kde4_automoc(${man2html_SRCS})


kde4_add_test_executable(man2html ${man2html_SRCS})
set_target_properties(man2html PROPERTIES COMPILE_FLAGS -DSIMPLE_MAN2HTML)
target_link_libraries(man2html  ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(kio_man_PART_SRCS man2html.cpp kio_man.cpp )

kde4_automoc(${kio_man_PART_SRCS})

kde4_add_plugin(kio_man ${kio_man_PART_SRCS})


target_link_libraries(kio_man  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDECORE_LIBS} )

install(TARGETS kio_man  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kmanpart_PART_SRCS kmanpart.cpp )

kde4_automoc(${kmanpart_PART_SRCS})

kde4_add_plugin(kmanpart WITH_PREFIX ${kmanpart_PART_SRCS})


target_link_libraries(kmanpart  ${KDE4_KPARTS_LIBS} ${KDE4_KHTML_LIBS} )

install(TARGETS kmanpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kio_man.css  DESTINATION  ${DATA_INSTALL_DIR}/kio_man )
install( FILES man.protocol kmanpart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

