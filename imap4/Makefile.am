INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

kde_module_LTLIBRARIES = kio_imap4.la

kio_imap4_la_SOURCES = imap4.cc base64md5.cc response.cc md5.cc
kio_imap4_la_LIBADD  = $(LIB_KIO)
kio_imap4_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = imap4.h

kdelnk_DATA = imap4.protocol imaps.protocol
kdelnkdir = $(kde_servicesdir)

