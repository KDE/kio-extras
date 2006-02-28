kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/systemview FILES home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop )

kde4_footer()



#original Makefile.am contents follow:

#systemviewdir = $(kde_datadir)/systemview
#systemview_DATA = home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop
#
#bin_SCRIPTS = kio_system_documenthelper
#
