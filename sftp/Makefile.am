## Makefile.am of kdebase/kioslave/sftp

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

####### Files

kde_module_LTLIBRARIES = kio_sftp.la

kio_sftp_la_SOURCES = process.cpp atomicio.cpp kio_sftp.cpp sftpfileattr.cpp ksshprocess.cpp
kio_sftp_la_LIBADD = $(LIB_KIO)
kio_sftp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = atomicio.h kio_sftp.h ksshprocess.h process.h sftpfileattr.h sftp.h

kdelnk_DATA = sftp.protocol
kdelnkdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_sftp.pot
