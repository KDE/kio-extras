


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/systemview FILES home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop )
install(PROGRAMS kio_system_documenthelper DESTINATION bin)




#original Makefile.am contents follow:

#systemviewdir = $(kde_datadir)/systemview
#systemview_DATA = home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop
#
#bin_SCRIPTS = kio_system_documenthelper
#
