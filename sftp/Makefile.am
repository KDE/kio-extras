####### kdevelop will overwrite this part!!! (begin)##########
kde_module_LTLIBRARIES = kio_sftp.la


INCLUDES = $(all_includes)


LDFLAGS = 

kio_sftp_la_METASOURCES=AUTO

kio_sftp_la_SOURCES = process.cpp atomicio.cpp kio_sftp.cpp sftpfileattr.cpp
kio_sftp_la_LIBADD = -lkio -lqt -lkio -lkdeui -lkdecore -lXext -lX11

kio_sftp_la_LDFLAGS = -module $(KDE_PLUGIN)  



EXTRA_DIST = sftp.h sftp.protocol sftpfileattr.cpp sftpfileattr.h kio_sftp.h kio_sftp.cpp atomicio.h atomicio.cpp process.cpp process.h kprocessblockingrw.cpp kprocessblockingrw.h 

install-data-local:
	$(mkinstalldirs) 
	$(INSTALL_DATA) $(srcdir)/sftp.protocol $(kde_servicesdir)

uninstall-local:
	-rm -f $(kde_servicesdir)

####### kdevelop will overwrite this part!!! (end)############
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

METASOURCES = AUTO
kio_sftp_la_LIBADD = -lkio
kio_sftp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
#messages:
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
#	if test -n "$$LIST"; then \
#		 $(XGETTEXT) -C -ki18n -x $(kde_includes)/kde.pot $$LIST -o ../po/sftp.pot; \
#	fi
#
