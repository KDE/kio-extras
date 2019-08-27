add_definitions(-DTRANSLATION_DOMAIN=\"kio5_recentlyused\")

set(
    kio_recentlyused_SRCS
    recentlyused.cpp
)

ecm_qt_declare_logging_category(kio_recentlyused_SRCS
                                HEADER recentlyused-logsettings.h
                                IDENTIFIER KIO_RECENTLYUSED_LOG
CATEGORY_NAME log_kio_recentlyused)

add_library(recentlyused MODULE ${kio_recentlyused_SRCS})

target_link_libraries(recentlyused
    KF5::KIOCore
    KF5::I18n
    KF5::Activities
    KF5::ActivitiesStats)
set_target_properties(recentlyused PROPERTIES OUTPUT_NAME "recentlyused")
set_target_properties(recentlyused PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio")
install(TARGETS recentlyused DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
