## Makefile.am of kdebase/kioslave/ldap

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO) $(LIBLDAP)

####### Files

bin_PROGRAMS = kio_ldap

kio_ldap_SOURCES = kio_ldap.cpp kldap.cpp kldapurl.cpp
noinst_HEADERS = kio_ldap.h kldap.h kldapurl.h

kioconfig_DATA = ldap.desktop
kioconfigdir = $(kde_confdir)/protocols

