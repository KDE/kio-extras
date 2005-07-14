INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)
METASOURCES = AUTO

CXXFLAGS=
if include_media_linuxcdpolling
CXXFLAGS+=-DCOMPILE_LINUXCDPOLLING
endif
if include_media_halbackend
CXXFLAGS+=-DCOMPILE_HALBACKEND
endif

kde_module_LTLIBRARIES = kcm_media.la
kcm_media_la_SOURCES = notifiermodule.cpp notifiermoduleview.ui \
                       serviceconfigdialog.cpp serviceview.ui \
                       mimetypelistboxitem.cpp \
                       managermodule.cpp managermoduleview.ui \
                       main.cpp

kcm_media_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined
kcm_media_la_LIBADD = ../libmediacommon/libmediacommon.la $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KIO)

noinst_HEADERS = notifiermodule.h managermodule.h \
                 serviceconfigdialog.h mimetypelistboxitem.h \
                 main.h

xdg_apps_DATA = media.desktop

messages: rc.cpp
	$(XGETTEXT) -kni18n $(kcm_media_la_SOURCES) -o $(podir)/kcmmedia.pot
