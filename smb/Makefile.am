## Makefile.am of kdebase/kioslave/smb

## Will install a version of libsmb if necessary
if COMPILE_LIBSMB
SUBDIRS = libsmb
LDADD	=	$(LIB_KIO) libsmb/src/libsmb.la
else
LDADD	=	$(LIB_KIO) $(LIBSMB)
endif

INCLUDES=	$(all_includes)
LDFLAGS =   $(all_libraries) $(KDE_RPATH)


####### Files

bin_PROGRAMS = kio_smb

kio_smb_SOURCES = kio_smb.cc
noinst_HEADERS = kio_smb.h

kioconfig_DATA = smb.desktop
kioconfigdir = $(kde_confdir)/protocols

messages:
