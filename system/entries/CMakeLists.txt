


########### install files ###############

install( FILES home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop  DESTINATION  ${DATA_INSTALL_DIR}/systemview )
install(PROGRAMS kio_system_documenthelper DESTINATION ${BIN_INSTALL_DIR})




#original Makefile.am contents follow:

#systemviewdir = $(kde_datadir)/systemview
#systemview_DATA = home.desktop documents.desktop users.desktop media.desktop remote.desktop trash.desktop
#
#bin_SCRIPTS = kio_system_documenthelper
#
