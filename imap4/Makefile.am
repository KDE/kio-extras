INCLUDES= -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

kde_module_LTLIBRARIES = kio_imap4.la

kio_imap4_la_SOURCES = 	imapcommand.cc imaplist.cc mailaddress.cc \
  mimeheader.cc rfcdecoder.cc imap4.cc imapinfo.cc imapparser.cc mailheader.cc \
  mimehdrline.cc  mimeio.cc
kio_imap4_la_LIBADD  = -lkdesasl $(LIB_KIO)
kio_imap4_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module $(KDE_PLUGIN)

noinst_HEADERS = imap4.h
EXTRA_DIST = README

kdelnk_DATA = imap4.protocol imaps.protocol
kdelnkdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cc -o $(podir)/kio_imap4.pot

include ../../admin/Doxyfile.am
