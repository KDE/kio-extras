add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(kiod_kmtpd_SRCS
    kmtpd.cpp
    mtpdevice.cpp
    mtpstorage.cpp
)

include_directories(${MTP_INCLUDE_DIR} "../shared")

qt5_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.daemon.xml kmtpd.h KMTPd)
qt5_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.device.xml mtpdevice.h MTPDevice)
qt5_add_dbus_adaptor(kiod_kmtpd_SRCS ../shared/org.kde.kmtp.storage.xml mtpstorage.h MTPStorage)

kcoreaddons_add_plugin(kmtpd INSTALL_NAMESPACE "kf5/kiod" JSON kmtpd.json SOURCES ${kiod_kmtpd_SRCS})

target_link_libraries(kmtpd ${MTP_LIBRARIES} kmtp Qt5::Core KF5::CoreAddons KF5::DBusAddons KF5::Solid)

kdbusaddons_generate_dbus_service_file(kiod5 org.kde.kmtp.daemon ${KDE_INSTALL_FULL_LIBEXECDIR_KF5})
