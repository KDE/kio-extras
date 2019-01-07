# shared-mime-info 0.40 is mandatory for generic-icon
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.40" )
find_package( SharedMimeInfo REQUIRED )

install( FILES network.xml  DESTINATION ${KDE_INSTALL_MIMEDIR} RENAME kf5_network.xml )
update_xdg_mimetypes( ${KDE_INSTALL_MIMEDIR} )
