
INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

kde_module_LTLIBRARIES = kio_smtp.la

kio_smtp_la_SOURCES = smtp.cc request.cc response.cc capabilities.cc command.cc transactionstate.cc
kio_smtp_la_LIBADD  = -lkdesasl $(LIB_KIO)
kio_smtp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = smtp.h request.h response.h capabilities.h command.h transactionstate.h

kdelnk_DATA = smtp.protocol smtps.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = test_headergeneration test_responseparser test_commands

TESTS = $(check_PROGRAMS)

test_headergeneration_SOURCES = test_headergeneration.cc
test_headergeneration_LDADD =  $(LIB_KDECORE)
test_headergeneration_LDFLAGS = $(all_libraries)

test_responseparser_SOURCES = test_responseparser.cc
test_responseparser_LDADD =  $(LIB_KDECORE)
test_responseparser_LDFLAGS = $(all_libraries)

test_commands_SOURCES = test_commands.cc
test_commands_LDADD =  $(kio_smtp_la_LIBADD)
test_commands_LDFLAGS = $(all_libraries)

messages:
	$(XGETTEXT) *.cc -o $(podir)/kio_smtp.pot
