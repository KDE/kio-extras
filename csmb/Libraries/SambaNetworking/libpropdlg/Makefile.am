INCLUDES = $(KDE_INCLUDES) $(QT_INCLUDES) $(X_INCLUDES) -I../mwn -I../mwn/qt2compat -I../CorelExplorer -I../../printutil -I../../ccui
#LIBADD = -L$(topdir)/SambaNetworking/mwn/.libs -L$(topdir)/SambaNetworking/libpropdlg/.libs $(LIB_KDEUI)  $(LIB_KDECORE) -lmwn -lprintutil
EXTRA_DIST =
lib_LTLIBRARIES = libpropdlg.la

include_HEADERS =

noinst_HEADERS = \
	DevicePropGeneral.h \
	DevicePropGeneralData.h	\
	FSPropGeneral.h	\
	FSPropGeneralData.h \
	FTPFilePropGeneral.h \
	FilePermissions.h \
	FilePermissionsData.h \
	FilePropGeneral.h \
	FilePropGeneralData.h \
	NetFilePropGeneral.h \
	NetFilePropGeneralData.h \
	PropDialog.h \
	PropDialogData.h \
	propres.h \
	ServPropGeneral.h \
	ServPropGeneralData.h \
	SharePropGeneral.h \
	SharePropGeneralData.h \
	TrashEntryPropGeneral.h	\
	TrashEntryPropGeneralData.h

libpropdlg_la_SOURCES = \
	DevicePropGeneral.cpp \
	DevicePropGeneralData.cpp \
	FSPropGeneral.cpp \
	FSPropGeneralData.cpp \
	FTPFilePropGeneral.cpp \
	FilePermissions.cpp \
	FilePermissionsData.cpp	\
	FilePropGeneral.cpp \
	FilePropGeneralData.cpp	\
	NetFilePropGeneral.cpp \
	NetFilePropGeneralData.cpp \
	PropDialog.cpp \
	PropDialogData.cpp \
	propres.cpp \
	ServPropGeneral.cpp \
	ServPropGeneralData.cpp	\
	SharePropGeneral.cpp \
	SharePropGeneralData.cpp \
	TrashEntryPropGeneral.cpp \
	TrashEntryPropGeneralData.cpp


libpropdlg_la_LDFLAGS = -version-info 2:0 --enable-shared
libpropdlg_la_LIBADD =-L../mwn -L../../printutil $(LIB_KDEUI)  $(LIB_KDECORE) -lmwn -lprintutil

libpropdlg_la_METASOURCES = USE_AUTOMOC

enable_shared=yes

messages:	stringres_prop.h resourcegen.cpp
	g++ -o resourcegen resourcegen.cpp
	./resourcegen > ../../po/propdlg.pot
	rm -f ./resourcegen

