AM_CPPFLAGS = -UQT_NO_ASCII_CAST
INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

if SSMTP
DO_SSMTP = kio_ssmtp.la
DO_SSMTP_PROTOCOL = ssmtp.protocol
else
DO_SSMTP = 
DO_SSMTP_PROTOCOL = 
endif

lib_LTLIBRARIES = kio_smtp.la $(DO_SSMTP)

kio_smtp_la_SOURCES = smtp.cc
kio_smtp_la_LIBADD  = $(LIB_KIO)
kio_smtp_la_LDFLAGS = $(all_libraries) -module -avoid-version

kio_ssmtp_la_SOURCES = ssmtp.cc
kio_ssmtp_la_LIBADD  = $(LIB_KIO) $(LIBSSL)
kio_ssmtp_la_LDFLAGS = $(all_libraries) $(SSL_LDFLAGS) -module -avoid-version

noinst_HEADERS = smtp.h

kdelnk_DATA = smtp.protocol $(DO_SSMTP_PROTOCOL)
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_smtp_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_smtp_la_LIBADD)
linktest_LDADD = $(kio_ssmtp_la_LIBADD)

ssmtp.cc: $(srcdir)/smtp.cc
	echo "#define SSMTP" >ssmtp.cc
	cat $(srcdir)/smtp.cc >>ssmtp.cc

CLEANFILES = ssmtp.cc

