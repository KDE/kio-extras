# $Id$

INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

kde_module_LTLIBRARIES = kio_smtp.la

kio_smtp_la_SOURCES = smtp.cc emailsettings.cpp
kio_smtp_la_LIBADD  = $(LIB_KIO)
kio_smtp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = smtp.h emailsettings.h

kdelnk_DATA = smtp.protocol
kdelnkdir = $(kde_servicesdir)
