## Makefile.am of kdebase/kioslave/ldap

INCLUDES=	$(all_includes) $(LDAP_INCS)
LDFLAGS =       $(all_libraries) $(KDE_RPATH) $(LDAP_RPATH)
LDADD	=	$(LIB_KIO) $(LDAP_LIBS)

####### Files

lib_LTLIBRARIES = kio_ldap.la

kio_ldap_la_SOURCES = kio_ldap.cpp kldap.cpp kldapurl.cpp
kio_ldap_la_LIBADD = $(LIB_KIO) $(LDAP_LIBS) $(KERBEROS_LIBS)
kio_ldap_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LDAP_RPATH) $(KRB_RPATH) -module -avoid-version

noinst_HEADERS = kio_ldap.h kldap.h kldapurl.h

kioconfig_DATA = ldap.desktop
kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_ldap_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) $(KDE_RPATH) $(LDAP_RPATH) -module -avoid-version
linktest_LDADD = $(kio_ldap_la_LIBADD)
