kde_module_LTLIBRARIES = kded_systemdirnotify.la

METASOURCES = AUTO
INCLUDES = $(all_includes)

kded_systemdirnotify_la_SOURCES = systemdirnotify.cpp systemdirnotify.skel systemdirnotifymodule.cpp systemdirnotifymodule.skel
kded_systemdirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_systemdirnotify_la_LIBADD = $(LIB_KSYCOCA)


servicesdir = $(kde_servicesdir)/kded
services_DATA = systemdirnotify.desktop

