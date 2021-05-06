include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. )

########### next target ###############

set(kio_man_test_SRCS 
    kio_man_test.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../request_hash.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../kio_man.cpp 
)
ecm_gperf_generate(../requests.gperf ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h kio_man_test_SRCS)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h PROPERTY SKIP_AUTOMOC ON)



add_executable(kio_man_test ${kio_man_test_SRCS})
ecm_mark_as_test(kio_man_test)
set_target_properties(kio_man_test PROPERTIES COMPILE_FLAGS "-DSIMPLE_MAN2HTML -DKIO_MAN_TEST")
target_link_libraries(kio_man_test Qt5::Widgets KF5::I18n KF5::KIOCore KF5::Archive KF5::Codecs Qt5::Network kio_man_debug)
if (AVOID_NULLPTR_WARNING_FLAG)
    target_compile_options(kio_man_test PRIVATE ${AVOID_NULLPTR_WARNING_FLAG})
endif()



########### next target ###############

set(man2html_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../request_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../man2html.cpp 
)
ecm_gperf_generate(../requests.gperf ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h man2html_SRCS)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h PROPERTY SKIP_AUTOMOC ON)


add_executable(man2html ${man2html_SRCS})
ecm_mark_as_test(man2html)
set_target_properties(man2html PROPERTIES COMPILE_FLAGS -DSIMPLE_MAN2HTML)
target_link_libraries(man2html Qt5::Core KF5::Codecs KF5::Archive kio_man_debug)
if (AVOID_NULLPTR_WARNING_FLAG)
    target_compile_options(man2html PRIVATE ${AVOID_NULLPTR_WARNING_FLAG})
endif()
