add_definitions(-DTRANSLATION_DOMAIN=\"kio5_man\")

find_package(KF5 REQUIRED COMPONENTS Codecs)

# silence warnings for gperf generating code with 0 for null pointers
set(AVOID_NULLPTR_WARNING_FLAG)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_flag "-Wno-zero-as-null-pointer-constant")
    check_cxx_compiler_flag(${_flag} _flag_present)
    if(_flag_present)
        set(AVOID_NULLPTR_WARNING_FLAG ${_flag})
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

# intermediate target/library, also used by the tests
add_library(kio_man_debug OBJECT)
target_link_libraries(kio_man_debug
    PRIVATE Qt5::Core
)
ecm_qt_declare_logging_category(kio_man_debug
    HEADER kio_man_debug.h
    IDENTIFIER KIO_MAN_LOG
    CATEGORY_NAME log_kio_man
)

########### next target ###############

set(kio_man_PART_SRCS man2html.cpp kio_man.cpp request_hash.cpp)
ecm_gperf_generate(requests.gperf ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h kio_man_PART_SRCS)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h PROPERTY SKIP_AUTOMOC ON)
add_library(kio_man MODULE ${kio_man_PART_SRCS})

if (AVOID_NULLPTR_WARNING_FLAG)
    target_compile_options(kio_man PRIVATE ${AVOID_NULLPTR_WARNING_FLAG})
endif()

target_link_libraries(kio_man
    PRIVATE KF5::KIOCore Qt5::Widgets KF5::Archive KF5::I18n KF5::Codecs Qt5::Network kio_man_debug
)
set_target_properties(kio_man PROPERTIES OUTPUT_NAME "man")

install(TARGETS kio_man DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
