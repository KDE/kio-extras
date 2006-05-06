kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/systemview FILES home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop )
install(PROGRAMS kio_system_documenthelper DESTINATION bin)

kde4_footer()



#original Makefile.am contents follow:

#systemviewdir = $(kde_datadir)/systemview
#systemview_DATA = home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop
#
#bin_SCRIPTS = kio_system_documenthelper
#
