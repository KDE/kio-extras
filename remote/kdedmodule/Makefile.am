kde_module_LTLIBRARIES = kded_remotedirnotify.la

METASOURCES = AUTO
INCLUDES = $(all_includes)

kded_remotedirnotify_la_SOURCES = remotedirnotify.cpp remotedirnotify.skel remotedirnotifymodule.cpp remotedirnotifymodule.skel
kded_remotedirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_remotedirnotify_la_LIBADD = $(LIB_KSYCOCA)


servicesdir = $(kde_servicesdir)/kded
services_DATA = remotedirnotify.desktop

