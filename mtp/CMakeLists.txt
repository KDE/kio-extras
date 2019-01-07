add_definitions(-DTRANSLATION_DOMAIN=\"kio5_mtp\")

add_definitions(-DQT_NO_CAST_FROM_ASCII)

add_subdirectory(shared)
add_subdirectory(kiod_module)

set(kio_mtp_PART_SRCS
    kio_mtp.cpp
)

include_directories("shared")
add_library(kio_mtp MODULE ${kio_mtp_PART_SRCS} )
target_link_libraries(kio_mtp kmtp Qt5::Core KF5::KIOCore KF5::I18n KF5::DBusAddons)
set_target_properties(kio_mtp PROPERTIES OUTPUT_NAME "mtp")

install(TARGETS kio_mtp DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### install files ###############

install( FILES mtp-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqueror/dirtree/remote )
install( FILES solid_mtp.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/solid/actions )
install( FILES mtp.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES mtp-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/remoteview )
