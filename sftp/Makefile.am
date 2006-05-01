## Makefile.am of kdebase/kioslave/sftp

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

####### Files

check_PROGRAMS = ksshprocesstest

ksshprocesstest_SOURCES = ksshprocesstest.cpp
ksshprocesstest_LDADD = $(LIB_KSYCOCA) ksshprocess.lo process.lo atomicio.lo

kde_module_LTLIBRARIES = kio_sftp.la

kio_sftp_la_SOURCES = process.cpp atomicio.cpp kio_sftp.cpp sftpfileattr.cpp ksshprocess.cpp
kio_sftp_la_LIBADD = $(LIB_KIO)
kio_sftp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = atomicio.h kio_sftp.h ksshprocess.h process.h sftpfileattr.h sftp.h

kdelnk_DATA = sftp.protocol
kdelnkdir = $(kde_servicesdir)

