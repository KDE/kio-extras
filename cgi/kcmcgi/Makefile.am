# $Id$

kde_module_LTLIBRARIES = kcm_cgi.la

kcm_cgi_la_SOURCES = kcmcgi.cpp
kcm_cgi_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_cgi_la_LIBADD = -lkdeui $(LIB_KIO) $(LIB_KFILE)

INCLUDES= $(all_includes) 

kcm_cgi_la_METASOURCES = AUTO

noinst_HEADERS = kcmcgi.h

KDE_ICON = cgi

data_DATA = kcmcgi.desktop
datadir = $(kde_appsdir)/Settings/System

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmcgi.pot
