add_definitions(-DTRANSLATION_DOMAIN=\"kio_mtp\")

add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(kio_mtp_PART_SRCS
    devicecache.cpp
    filecache.cpp
    kio_mtp.cpp
    kio_mtp_helpers.cpp
)

include_directories(${MTP_INCLUDE_DIR})
add_library(kio_mtp MODULE ${kio_mtp_PART_SRCS} )
target_link_libraries(kio_mtp ${MTP_LIBRARIES} Qt5::Core KF5::KIOCore KF5::I18n KF5::Solid)
set_target_properties(kio_mtp PROPERTIES OUTPUT_NAME "mtp")

install(TARGETS kio_mtp DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

########### install files ###############

install( FILES mtp-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES solid_mtp.desktop  DESTINATION  ${DATA_INSTALL_DIR}/solid/actions )
install( FILES mtp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES mtp-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/remoteview )
