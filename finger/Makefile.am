## Makfile.am for kio_finger
## Edit from Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

bin_PROGRAMS = KDEfinger

KDEfinger_SOURCES = KDEfingerMain.cpp KDEfinger.cpp fingerSocket.cpp
KDEfinger_LDFLAGS = $(all_libraries) $(KDE_RPATH)
KDEfinger_LDADD = $(LIB_KFILE)   

lib_LTLIBRARIES = kio_finger.la

kio_finger_la_SOURCES = kio_finger.cpp 
kio_finger_la_LIBADD = -lkio
kio_finger_la_LDFLAGS = -module -avoid-version -no-undefined

noinst_HEADERS = kio_finger.h KDEfinger.h fingerSocket.h

kdelnk_DATA = finger.protocol
kdelnkdir = $(kde_servicesdir)

data_DATA = finger.pl fingerHead.template fingerTail konqi.png
datadir = $(kde_datadir)/kfinger/kio_finger
EXTRA_DIST=$(data_DATA)   

METASOURCES = AUTO

messages:
	$(XGETTEXT) $(kfinger_SOURCES) -o $(podir)/kio_finger.pot

#Problem: The paths to pictures in the templates are static. We must adjust 
#them to reflect the directory where they are installed.
install-data-local:
	@echo
	@echo "Setting up the templates"
	@echo
	@echo "Problem: The paths to pictures in the templates are static." 
	@echo "We must adjust them to reflect the directory where they are installed."
	@(perl setupTemplates.pl $(kde_datadir)/kfinger/kio_finger)
	@echo
	@echo "done"
