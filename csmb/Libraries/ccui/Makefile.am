SUBDIRS = . tests

INCLUDES= $(QT_INCLUDES) $(KDE_INCLUDES) $(X_INCLUDES) 

CXXFLAGS     = -O2 -fno-exceptions -frtti -fno-check-new -Wall -W -Wpointer-arith -Wmissing-prototypes -Wwrite-strings -Wno-long-long -fno-builtin

lib_LTLIBRARIES = libccui.la

include_HEADERS = \
	ccui_common.h \
	header.h listview.h kdelistview.h \
	ipedit.h \
	wizard.h wizardpage.h \
	cdate.h \
	corellistboxitem.h \
	flattoolbar.h \
	clistviewedit.h

noinst_HEADERS =

libccui_la_SOURCES = \
	header.cpp listview.cpp kdelistview.cpp \
	ipedit.cpp \
	wizard.cpp wizardpage.cpp \
	cdate.cpp \
	corellistboxitem.cpp \
	flattoolbar.cpp \
	clistviewedit.cpp

libccui_la_LDFLAGS = -version-info 1:0 

libccui_la_METASOURCES = USE_AUTOMOC
