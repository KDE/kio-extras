INCLUDES = -I$(srcdir)/../libmediacommon -I../libmediacommon $(all_includes)

kde_module_LTLIBRARIES = kded_medianotifier.la
kded_medianotifier_la_LDFLAGS = -module -avoid-version $(all_libraries)
kded_medianotifier_la_LIBADD = ../libmediacommon/libmediacommon.la $(LIB_KDECORE) \
                               $(LIB_KDEUI) $(LIB_KIO)
kded_medianotifier_la_SOURCES = medianotifier.cpp medianotifier.skel \
                                notificationdialog.cpp notificationdialogview.ui

noinst_HEADERS = medianotifier.h notificationdialog.h

METASOURCES = AUTO

services_DATA = medianotifier.desktop
servicesdir = $(kde_servicesdir)/kded

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kay.pot
