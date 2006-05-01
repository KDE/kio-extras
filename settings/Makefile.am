INCLUDES= $(all_includes)
KDE_CXXFLAGS = -UQT3_SUPPORT
####### Files

kde_module_LTLIBRARIES = kio_settings.la

kio_settings_la_SOURCES = kio_settings.cc
kio_settings_la_LIBADD  = $(LIB_KSYCOCA)
kio_settings_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_settings.h

kdelnk_DATA = settings.protocol programs.protocol applications.protocol
kdelnkdir = $(kde_servicesdir)


SUBDIRS=. 


