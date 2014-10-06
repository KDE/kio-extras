include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

set(kio_man_test_SRCS 
    kio_man_test.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../request_hash.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../kio_man.cpp )



add_executable(kio_man_test ${kio_man_test_SRCS})
ecm_mark_as_test(kio_man_test)
set_target_properties(kio_man_test PROPERTIES COMPILE_FLAGS "-DSIMPLE_MAN2HTML -DKIO_MAN_TEST")
target_link_libraries(kio_man_test KF5::KIOCore KF5::Archive KF5::KDELibs4Support)


########### next target ###############

set(man2html_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../request_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp )


add_executable(man2html ${man2html_SRCS})
ecm_mark_as_test(man2html)
set_target_properties(man2html PROPERTIES COMPILE_FLAGS -DSIMPLE_MAN2HTML)
target_link_libraries(man2html KF5::KDELibs4Support)

