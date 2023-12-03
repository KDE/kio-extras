add_definitions(-DTRANSLATION_DOMAIN=\"kio5_mtp\")

cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_INCLUDES Libmtp::Libmtp)
list(APPEND CMAKE_REQUIRED_LIBRARIES Libmtp::Libmtp)
check_symbol_exists(LIBMTP_Get_Children "libmtp.h" HAVE_LIBMTP_Get_Children)
cmake_pop_check_state()

add_subdirectory(shared)
add_subdirectory(kiod_module)

add_library(kio_mtp MODULE)
set_target_properties(kio_mtp PROPERTIES
    OUTPUT_NAME "mtp"
)

target_sources(kio_mtp PRIVATE
    kio_mtp.cpp
)

target_include_directories(kio_mtp PRIVATE shared)
target_link_libraries(kio_mtp kmtp Qt::Core KF5::KIOCore KF5::I18n KF5::DBusAddons)

ecm_qt_declare_logging_category(kio_mtp
    HEADER kio_mtp_debug.h
    IDENTIFIER LOG_KIO_MTP
    CATEGORY_NAME kf.kio.workers.mtp
    OLD_CATEGORY_NAMES kde.kio-mtp kf.kio.slaves.mtp
    DESCRIPTION "KIO mtp"
    EXPORT KIO_EXTRAS
)

install(TARGETS kio_mtp DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### install files ###############

if (NOT KF6_COMPAT_BUILD)
    install( FILES mtp-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqueror/dirtree/remote )
    install( FILES solid_mtp.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/solid/actions )
    install( FILES mtp-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/remoteview )
endif()
