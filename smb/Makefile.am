## Makefile.am of kdebase/kioslave/man

INCLUDES=$(all_includes)
AM_LDFLAGS =$(all_libraries) 
####### Files

kde_module_LTLIBRARIES = kio_smb.la
kio_smb_la_SOURCES = kio_smb.cpp \
             kio_smb_auth.cpp \
             kio_smb_browse.cpp \
             kio_smb_config.cpp \
             kio_smb_dir.cpp \
             kio_smb_file.cpp \
             kio_smb_internal.cpp \
	     kio_smb_mount.cpp

kio_smb_la_LIBADD = -lkio -lsmbclient $(SMBCLIENT_EXTRA_LIBS)

kio_smb_la_LDFLAGS = -module $(KDE_PLUGIN)

noinst_HEADERS = kio_smb.h kio_smb_internal.h

kdelnk_DATA = smb.protocol

kdelnkdir = $(kde_servicesdir)

dirtree_DATA = smb-network.desktop
dirtreedir = $(kde_datadir)/konqueror/dirtree/remote

remote_DATA = smb-network.desktop
remotedir = $(kde_datadir)/remoteview

METASOURCES = AUTO

mimetypedir = $(kde_mimedir)/application
mimetype_DATA = x-smb-workgroup.desktop x-smb-server.desktop

