## Makefile.am of kdebase/kioslave/smb

AM_CPPFLAGS = -UQT_NO_ASCII_CAST

SUBDIRS=libsmb++

INCLUDES=	$(all_includes)
LDFLAGS =   $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_smb.la

kio_smb_la_SOURCES = smb.cc
kio_smb_la_LIBADD = -lkio libsmb++/src/libsmb++.la @EXTRA_LIBSMB_ADD@
kio_smb_la_LDFLAGS = -module -avoid-version @EXTRA_LIBSMB_FLAGS@
noinst_HEADERS = smb.h

if LINKHOOK
install-exec-hook:
	rm -f @kde_libraries@/libsmb.so*
	rm -f @kde_libraries@/libnmb.so*
	rm -f @kde_libraries@/libsamba.so*
	rm -f @kde_libraries@/libmsrpc.so*
	ln -s @SAMBA_LIBS@/libsmb.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libnmb.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libsamba.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libmsrpc.so* @kde_libraries@
else
install-exec-hook:
endif

kioconfig_DATA = smb.desktop
kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_smb_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_smb_la_LIBADD)
