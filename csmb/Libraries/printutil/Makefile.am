INCLUDES = -DQT_20 $(all_includes) -I../ccui
EXTRA_DIST =
lib_LTLIBRARIES = libprintutil.la
INSTALL_PROGRAM = @INSTALL@ $(SETUIDFLAGS)$(SETGIDFLAGS)
include_HEADERS =

noinst_HEADERS = \
	printerobject.h \
	manufacture.h \
	advanceddlg.h \
	pageOne.h \
	pageThree.h \
	generaldlg.h \
	pageTwo.h \
	pageFour.h \
	pageFive.h \
	outputdlg.h \
	constText.h \
	commonfunc.h  \
	sharedlg.h \
	mywizard.h \
	rename.h

libprintutil_la_SOURCES = \
	printerobject.cpp \
	manufacture.cpp \
	advanceddlg.cpp \
	pageThree.cpp \
	pageTwo.cpp \
	generaldlg.cpp \
	outputdlg.cpp \
	pageOne.cpp \
	pageFour.cpp \
	pageFive.cpp \
	commonfunc.cpp \
	sharedlg.cpp \
	mywizard.cpp \
	rename.cpp



libprintutil_la_LDFLAGS = -version-info 1:0 --enable-shared
libprintutil_la_LIBADD =-L../ccui $(LIB_KDEUI) $(LIB_KDECORE) -lccui

libprintutil_la_METASOURCES = USE_AUTOMOC

enable_shared=yes
