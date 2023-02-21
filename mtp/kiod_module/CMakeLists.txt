set(kiod_kmtpd_SRCS
    kmtpd.cpp
    mtpdevice.cpp
    mtpstorage.cpp
    mtplister.cpp
)

qt_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.daemon.xml kmtpd.h KMTPd)
qt_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.device.xml mtpdevice.h MTPDevice)
qt_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.storage.xml mtpstorage.h MTPStorage)
qt_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.Lister.xml mtplister.h MTPLister)

kcoreaddons_add_plugin(kmtpd INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kiod" SOURCES ${kiod_kmtpd_SRCS})

target_link_libraries(kmtpd
    kmtp
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::DBusAddons
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::KIOCore
    Qt::Core
    Libmtp::Libmtp
)

ecm_qt_declare_logging_category(kmtpd
    HEADER kiod_kmtpd_debug.h
    IDENTIFIER LOG_KIOD_KMTPD
    CATEGORY_NAME kf.kiod.kmtpd.
    OLD_CATEGORY_NAMES kf5.kiod.kmtpd
    DESCRIPTION "KIO mtp (demon)"
    EXPORT KIO_EXTRAS
)

kdbusaddons_generate_dbus_service_file(kiod5 org.kde.kmtpd5 ${KDE_INSTALL_FULL_LIBEXECDIR_KF})
