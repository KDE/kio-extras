## Makefile.am of kdebase/kioslave/CorelSmb


INCLUDES= -DQT_20	$(all_includes)  -I./Libraries/SambaNetworking/mwn -I./Libraries/ccui -I$(QTDIR)
LDADD = $(LIB_KDECORE)
LDFLAGS =   $(all_libraries) $(KDE_RPATH)
####### Files

lib_LTLIBRARIES = kio_corel_smb.la
                                  
kio_corel_smb_la_SOURCES = corel_smb.cpp
kio_corel_smb_la_LIBADD = $(LIB_KDECORE)  -lkio -lccui -lmwn @EXTRA_LIBSMB_ADD@
kio_corel_smb_la_LDFLAGS = -module -avoid-version @EXTRA_LIBSMB_FLAGS@
noinst_HEADERS = corel_smb.h

if LINKHOOK
install-exec-hook:
	rm -f @kde_libraries@/libcorel_smb.so*
	rm -f @kde_libraries@/libnmb.so*
	rm -f @kde_libraries@/libsamba.so*
	rm -f @kde_libraries@/libmsrpc.so*
	ln -s @SAMBA_LIBS@/libcorel_smb.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libnmb.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libsamba.so* @kde_libraries@
	ln -s @SAMBA_LIBS@/libmsrpc.so* @kde_libraries@
else
install-exec-hook:
endif

kdelnk_DATA = smb.protocol
kdelnkdir = $(kde_servicesdir)

#kioconfig_DATA = smb.desktop
#kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_corel_smb_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_corel_smb_la_LIBADD)
