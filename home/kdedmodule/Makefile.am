kde_module_LTLIBRARIES = kded_homedirnotify.la

METASOURCES = AUTO
INCLUDES = $(all_includes)

kded_homedirnotify_la_SOURCES = homedirnotify.cpp homedirnotify.skel homedirnotifymodule.cpp homedirnotifymodule.skel
kded_homedirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_homedirnotify_la_LIBADD = $(LIB_KSYCOCA)


servicesdir = $(kde_servicesdir)/kded
services_DATA = homedirnotify.desktop

