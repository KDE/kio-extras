configure_file(config-mtp.h.cmake config-mtp.h @ONLY)

add_library(kmtp STATIC)

set_source_files_properties(org.kde.kmtp.storage.xml PROPERTIES
    INCLUDE kmtpfile.h
)

set_source_files_properties(org.kde.kmtp.Lister.xml PROPERTIES
    INCLUDE kmtpfile.h
)

set(kmtp_dbus_SRCS)
qt_add_dbus_interfaces(kmtp_dbus_SRCS
    org.kde.kmtp.daemon.xml
    org.kde.kmtp.device.xml
    org.kde.kmtp.storage.xml
    org.kde.kmtp.Lister.xml
)

target_sources(kmtp PRIVATE
    kmtpdinterface.cpp
    kmtpdeviceinterface.cpp
    kmtpstorageinterface.cpp
    kmtpfile.cpp
    ${kmtp_dbus_SRCS}
)
set_property(TARGET kmtp PROPERTY POSITION_INDEPENDENT_CODE ON)
# NOTE: do not link libmtp, we only want it used in the kiod_module, not the kio worker
target_link_libraries(kmtp Qt::Core Qt::DBus)
