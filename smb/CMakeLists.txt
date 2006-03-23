kde4_header()

project(kioslave-smb)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kio_smb_PART_SRCS 
   kio_smb.cpp 
   kio_smb_auth.cpp 
   kio_smb_browse.cpp 
   kio_smb_config.cpp 
   kio_smb_dir.cpp 
   kio_smb_file.cpp 
   kio_smb_internal.cpp 
   kio_smb_mount.cpp )

kde4_automoc(${kio_smb_PART_SRCS})

kde4_add_plugin(kio_smb ${kio_smb_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_smb )

target_link_libraries(kio_smb  ${KDE4_KDE3SUPPORT_LIBS} smbclient )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_smb )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/dirtree/remote FILES smb-network.desktop )
install_files( ${MIME_INSTALL_DIR}/application FILES x-smb-workgroup.desktop x-smb-server.desktop )
install_files( ${SERVICES_INSTALL_DIR} FILES smb.protocol )
install_files( ${DATA_INSTALL_DIR}/remoteview FILES smb-network.desktop )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/man
#
#INCLUDES=$(all_includes)
#AM_LDFLAGS =$(all_libraries) 
######## Files
#
#kde_module_LTLIBRARIES = kio_smb.la
#kio_smb_la_SOURCES = kio_smb.cpp \
#             kio_smb_auth.cpp \
#             kio_smb_browse.cpp \
#             kio_smb_config.cpp \
#             kio_smb_dir.cpp \
#             kio_smb_file.cpp \
#             kio_smb_internal.cpp \
#	     kio_smb_mount.cpp
#
#kio_smb_la_LIBADD = -lkio -lsmbclient $(SMBCLIENT_EXTRA_LIBS)
#
#kio_smb_la_LDFLAGS = -module $(KDE_PLUGIN)
#
#noinst_HEADERS = kio_smb.h kio_smb_internal.h
#
#kdelnk_DATA = smb.protocol
#
#kdelnkdir = $(kde_servicesdir)
#
#dirtree_DATA = smb-network.desktop
#dirtreedir = $(kde_datadir)/konqueror/dirtree/remote
#
#remote_DATA = smb-network.desktop
#remotedir = $(kde_datadir)/remoteview
#
#METASOURCES = AUTO
#
#mimetypedir = $(kde_mimedir)/application
#mimetype_DATA = x-smb-workgroup.desktop x-smb-server.desktop
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_smb.pot
