
kde_module_LTLIBRARIES = kcm_cgi.la

kcm_cgi_la_SOURCES = kcmcgi.cpp
kcm_cgi_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_cgi_la_LIBADD = -lkdeui $(LIB_KIO)

INCLUDES= $(all_includes) 

kcm_cgi_la_METASOURCES = AUTO

noinst_HEADERS = kcmcgi.h

xdg_apps_DATA = kcmcgi.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmcgi.pot
