

INCLUDES = -I$(top_srcdir)/ccui \
	  $(all_includes)

CXXFLAGS     = -O2 -fno-exceptions -frtti -fno-check-new -Wall -W -Wpointer-arith -Wmissing-prototypes -Wwrite-strings -Wno-long-long -fno-builtin

LDFLAGS = $(QT_LDFLAGS) $(KDE_LDFLAGS) $(X_LDFLAGS) $(KDE_RPATH)

# Add all sample names here
bin_PROGRAMS = listviews ipedittest wizardtest datetest listboxtest editlist

# Add all test headers here
noinst_HEADERS = listviews.h ipedittest.h wizardtest.h datetest.h listboxtest.h \
	editlist.h

METASOURCES = AUTO

#LDADD = ../libccui.la $(qt_libraries)/libqt.so.2 -lkdecore -lkdeui
LDADD = ../libccui.la -lqt -lkdecore -lkdeui

# Add all sources for each test here
listviews_SOURCES = listviews.cpp
ipedittest_SOURCES = ipedittest.cpp
wizardtest_SOURCES = wizardtest.cpp
datetest_SOURCES = datetest.cpp
listboxtest_SOURCES = listboxtest.cpp
editlist_SOURCES = editlist.cpp
