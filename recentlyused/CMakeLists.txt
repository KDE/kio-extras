add_definitions(-DTRANSLATION_DOMAIN=\"kio5_recentlyused\")

add_library(recentlyused MODULE)
set_target_properties(recentlyused PROPERTIES
    OUTPUT_NAME "recentlyused"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio"
)

target_sources(recentlyused PRIVATE
    recentlyused.cpp
)

ecm_qt_declare_logging_category(recentlyused
                                HEADER recentlyused-logsettings.h
                                IDENTIFIER KIO_RECENTLYUSED_LOG
CATEGORY_NAME log_kio_recentlyused)

target_link_libraries(recentlyused
    KF5::KIOCore
    KF5::I18n
    KF5::Activities
    KF5::ActivitiesStats)

install(TARGETS recentlyused DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
