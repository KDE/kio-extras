add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(kmtp_PART_SRCS
    kmtpdinterface.cpp
    kmtpdeviceinterface.cpp
    kmtpstorageinterface.cpp
    kmtpfile.cpp
)

set_source_files_properties(org.kde.kmtp.storage.xml
    PROPERTIES INCLUDE kmtpfile.h
)

qt5_add_dbus_interfaces(kmtp_PART_SRCS
    org.kde.kmtp.daemon.xml
    org.kde.kmtp.device.xml
    org.kde.kmtp.storage.xml
)

add_library(kmtp STATIC ${kmtp_PART_SRCS})
target_link_libraries(kmtp Qt5::Core Qt5::DBus)
