add_definitions(-DTRANSLATION_DOMAIN=\"kio5_afc\")

add_library(kio_afc MODULE)
set_target_properties(kio_afc PROPERTIES
    OUTPUT_NAME "afc"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf${QT_MAJOR_VERSION}/kio"
)

target_sources(kio_afc PRIVATE
    kio_afc.cpp
    afcurl.cpp
    afcutils.cpp
    afcdevice.cpp
    afcclient.cpp
    afcfile.cpp
    afcapp.cpp
    afcdiskusage.cpp
    afcfilereader.cpp
    afcspringboard.cpp
)

ecm_qt_declare_logging_category(kio_afc
    HEADER afc_debug.h
    IDENTIFIER KIO_AFC_LOG
    CATEGORY_NAME kf.kio.workers.afc
    DESCRIPTION "KIO afc"
    EXPORT KIO_EXTRAS
)

target_link_libraries(kio_afc
    KF5::KIOCore
    IMobileDevice::IMobileDevice
    PList::PList
    KF5::I18n
)

install(TARGETS kio_afc DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)

install(FILES solid_afc.desktop DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)
install(FILES afc-network.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/remoteview)
