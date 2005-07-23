INCLUDES = -I$(srcdir)/../libmediacommon -I../libmediacommon $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_media.la
kcm_media_la_SOURCES = notifiermodule.cpp notifiermoduleview.ui \
                       serviceconfigdialog.cpp serviceview.ui \
                       mimetypelistboxitem.cpp \
                       managermodule.cpp managermoduleview.ui \
                       main.cpp

kcm_media_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
kcm_media_la_LIBADD = ../libmediacommon/libmediacommon.la $(LIB_KIO)

noinst_HEADERS = notifiermodule.h managermodule.h \
                 serviceconfigdialog.h mimetypelistboxitem.h \
                 main.h

xdg_apps_DATA = media.desktop

messages: rc.cpp
	$(XGETTEXT) -kni18n $(kcm_media_la_SOURCES) -o $(podir)/kcmmedia.pot
