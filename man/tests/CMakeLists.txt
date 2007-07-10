
add_definitions (-DQT3_SUPPORT)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

set(kio_man_test_SRCS 
    kio_man_test.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../kio_man.cpp )



kde4_add_test_executable(kio_man_test ${kio_man_test_SRCS})
set_target_properties(kio_man_test PROPERTIES COMPILE_FLAGS "-DSIMPLE_MAN2HTML -DKIO_MAN_TEST")
target_link_libraries(kio_man_test  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})


########### next target ###############

set(man2html_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp )



kde4_add_test_executable(man2html ${man2html_SRCS})
set_target_properties(man2html PROPERTIES COMPILE_FLAGS -DSIMPLE_MAN2HTML)
target_link_libraries(man2html  ${KDE4_KDECORE_LIBS} )

