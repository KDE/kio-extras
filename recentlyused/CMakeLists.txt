add_definitions(-DTRANSLATION_DOMAIN=\"kio5_recentlyused\")

add_library(recentlyused MODULE)
set_target_properties(recentlyused PROPERTIES
    OUTPUT_NAME "recentlyused"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio"
)
target_sources(recentlyused PRIVATE
    recentlyused.cpp
)
# recentlyused.h has examples in the comment secions which match /*,
# so triggering the preprocessor to warn about nested comments.
# Using a pragma to ignore that diagnostic does not work for preprocessor warnings
# so for now simply that warning is completly disabled
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_flag "-Wno-comment")
    check_cxx_compiler_flag(${_flag} _flag_present)
    if(_flag_present)
        target_compile_options(recentlyused PRIVATE "${_flag}")
    endif()
endif()

ecm_qt_declare_logging_category(recentlyused
    HEADER recentlyused-logsettings.h
    IDENTIFIER KIO_RECENTLYUSED_LOG
    CATEGORY_NAME kf.kio.slaves.recentlyused
    OLD_CATEGORY_NAMES log_kio_recentlyused
    DESCRIPTION "KIO recentlyused"
    EXPORT KIO_EXTRAS
)

target_link_libraries(recentlyused
    KF5::KIOCore
    KF5::I18n
    KF5::Activities
    KF5::ActivitiesStats)

install(TARGETS recentlyused DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
