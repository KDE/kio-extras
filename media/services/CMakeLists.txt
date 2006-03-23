kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/servicemenus FILES media_mount.desktop media_unmount.desktop media_eject.desktop media_safelyremove.desktop )

kde4_footer()



#original Makefile.am contents follow:

#servicesdir = $(kde_datadir)/konqueror/servicemenus
#services_DATA = media_mount.desktop media_unmount.desktop media_eject.desktop media_safelyremove.desktop
#
