INCLUDES = $(all_includes) -I../mwn -I../libpropdlg -I../../printutil -I../../ccui

bin_PROGRAMS = CopyAgent

CopyAgent_SOURCES = main.cpp \
		CopyProgressDlgData.cpp \
		CopyProgressDlg.cpp 
		main.cpp

noinst_HEADERS = CopyProgressDlg.h \
		filedel.h \
		CopyProgressDlgData.h
		main.h

CopyAgent_LDADD = $(KDE_LDFLAGS) -L../../ccui -L../mwn -L../libpropdlg -L../../printutil -lmwn -lpropdlg -lccui -lprintutil -laps $(LIB_KFILE)
CopyAgent_METASOURCES = USE_AUTOMOC

