APPNAME=CorelExplorer
INCLUDES = $(all_includes) -I../mwn -I../mwn/qt2compat -I../libpropdlg -I../../printutil
#CXXFLAGS = -g
#LDFLAGS = -lqt $(all_libraries) $(KDE_RPATH)
LDADD = -L$(topdir)/SambaNetworking/mwn/.libs -L$(topdir)/SambaNetworking/libpropdlg/.libs $(LIB_KDEUI) $(LIB_KDECORE) -lksycoca -lmwn -lpropdlg -lkhtml -ljpeg -laps -lccui

bin_PROGRAMS = $(APPNAME)

noinst_HEADERS = tips.h rightpanel.h mainfrm.h \
		lefttree.h corellogo.h \
                DriveDiagram.h \
                explres.h automount.h printer.h printerroot.h printjob.h\
                DisconnectDlg.h DisconnectDlgData.h \
		cuturldrag.h topcombo.h \
		expcommon.h filerun.h device.h \
		home.h vdrive.h \
                almosthtmlview.h browsercache.h \
		dropselector.h waitkfm.h stringres.h \
		skiplist.h autotopcombo.h \
		history.h

CorelExplorer_SOURCES =	\
		almosthtmlview.cpp \
		automount.cpp \
		autotopcombo.cpp \
		browsercache.cpp \
		corellogo.cpp \
		cuturldrag.cpp \
		device.cpp \
                DisconnectDlg.cpp \
		DisconnectDlgData.cpp \
		DriveDiagram.cpp \
		dropselector.cpp \
		expcommon.cpp \
		explres.cpp \
                filerun.cpp \
                history.cpp \
		home.cpp \
		lefttree.cpp \
		main.cpp \
		mainfrm.cpp \
                printer.cpp \
		printerroot.cpp \
		printjob.cpp \
                rightpanel.cpp \
                tips.cpp \
		topcombo.cpp \
                waitkfm.cpp

CorelExplorer_METASOURCES = USE_AUTOMOC

SUBDIRS =

install-data-local:
	$(mkinstalldirs) $(kde_icondir) $(kde_icondir)/mini  $(kde_datadir) $(kde_datadir)/$(APPNAME)
	strip -s $(kde_bindir)/$(APPNAME)
	install $(srcdir)/$(APPNAME).xpm $(kde_icondir)
	install $(srcdir)/$(APPNAME)Mini.xpm $(kde_icondir)/mini/$(APPNAME).xpm
	install -m666 $(srcdir)/cfm.exclude $(kde_datadir)/$(APPNAME)/cfm.exclude

messages:
	g++ -o resourcegen resourcegen.cpp
	./resourcegen > ../../po/CorelExplorer.pot
	rm -f ./resourcegen

