## Makefile.am of kdebase/kioslave/smb

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO) $(LIBSMB)

####### Files

bin_PROGRAMS = kio_smb

kio_smb_SOURCES = kio_smb.cc
noinst_HEADERS = kio_smb.h

kioconfig_DATA = smb.desktop
kioconfigdir = $(kde_confdir)/protocols

messages:

