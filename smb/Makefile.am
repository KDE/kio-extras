## Makefile.am of kdebase/kioslave/smb

SUBDIRS=libsmb++

INCLUDES=	$(all_includes)
LDFLAGS =   $(all_libraries) $(KDE_RPATH)

####### Files

kde_module_LTLIBRARIES = kio_smb.la

kio_smb_la_SOURCES = smb.cc
kio_smb_la_LIBADD = -lkio libsmb++/src/libsmb++.la @EXTRA_LIBSMB_ADD@
kio_smb_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) @EXTRA_LIBSMB_FLAGS@
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

kdelnk_DATA = smb.protocol
kdelnkdir = $(kde_servicesdir)

