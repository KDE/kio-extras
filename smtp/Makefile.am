# $Id$

INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

kde_module_LTLIBRARIES = kio_smtp.la

kio_smtp_la_SOURCES = smtp.cc request.cc
kio_smtp_la_LIBADD  = -lkdesasl $(LIB_KIO)
kio_smtp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = smtp.h request.h

kdelnk_DATA = smtp.protocol smtps.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = test_headergeneration

TESTS = $(check_PROGRAMS)

test_headergeneration_SOURCES = test_headergeneration.cc
test_headergeneration_LDADD =  $(LIB_KDECORE)
test_headergeneration_LDFLAGS = $(all_libraries)

messages:
	$(XGETTEXT) *.cc -o $(podir)/kio_smtp.pot
