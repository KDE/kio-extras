## Makefile.am of kdebase/kioslave/smb

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO) $(LIB_SMB)

####### Files

bin_PROGRAMS = kio_smb

kio_smb_SOURCES = smb.cc
noinst_HEADERS = smb.h

kioconfig_DATA = smb.kdelnk
kioconfigdir = $(kde_confdir)/protocols

messages:

