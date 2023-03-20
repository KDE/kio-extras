add_library(kio_about MODULE)
set_target_properties(kio_about PROPERTIES
    OUTPUT_NAME "about"
)

target_sources(kio_about PRIVATE kio_about.cpp)

target_link_libraries(kio_about
    KF${QT_MAJOR_VERSION}::KIOCore
    Qt::Network
)

install(TARGETS kio_about  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio )
