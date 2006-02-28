kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kio_man_test_SRCS kio_man_test.cpp )

kde4_automoc(${kio_man_test_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(kio_man_test ${kio_man_test_SRCS})

target_link_libraries(kio_man_test  ${QT_AND_KDECORE_LIBS} kio kdeui )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(man2html_SRCS dummy.cpp )

kde4_automoc(${man2html_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(man2html ${man2html_SRCS})

target_link_libraries(man2html  ${QT_AND_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(kio_man_PART_SRCS man2html.cpp kio_man.cpp )

kde4_automoc(${kio_man_PART_SRCS})

kde4_add_plugin(kio_man ${kio_man_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_man )

target_link_libraries(kio_man  ${QT_AND_KDECORE_LIBS} kio ${QT_QT3SUPPORT_LIBRARY})

install_targets(${LIB_INSTALL_DIR}/kde4 kio_man )


########### next target ###############

set(kmanpart_PART_SRCS kmanpart.cpp )

kde4_automoc(${kmanpart_PART_SRCS})

kde4_add_plugin(kmanpart WITH_PREFIX ${kmanpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmanpart )

target_link_libraries(kmanpart  ${QT_AND_KDECORE_LIBS} khtml kparts kdeui kio)

install_targets(${LIB_INSTALL_DIR}/kde4 kmanpart )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kio_man FILES kio_man.css )
install_files( ${SERVICES_INSTALL_DIR} FILES man.protocol kmanpart.desktop )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/man
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#EXTRA_PROGRAMS = kio_man_test man2html
#
######## just for testing (j.habenicht@europemail.com, 15.02.2001)
#
#kio_man_test_SOURCES = kio_man_test.cpp
#kio_man_test_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kio_man_test_LDADD = man2html.lo kio_man.lo $(LIB_KIO) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_man.la  libkmanpart.la
#
#kio_man_la_SOURCES = man2html.cpp kio_man.cpp
#kio_man_la_LIBADD = $(LIB_KSYCOCA)
#kio_man_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = kio_man.h
#### TODO Why is man2htmk.h distributed?
#
#libkmanpart_la_SOURCES = kmanpart.cpp
#libkmanpart_la_LIBADD = -lkhtml $(LIB_KPARTS)
#libkmanpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
#
#kdelnk_DATA = man.protocol kmanpart.desktop
#kdelnkdir = $(kde_servicesdir)
#
#kio_man_data_DATA = kio_man.css
#kio_man_datadir = $(kde_datadir)/kio_man
#EXTRA_DIST=$(kio_man_data_DATA)
#
#METASOURCES = AUTO
#
#messages:
#	$(XGETTEXT) *.cpp *.h -o $(podir)/kio_man.pot
#
#man2html_SOURCES = dummy.cpp
#man2html_LDADD = man2html_simple.o $(LIB_QT)
#man2html_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#dummy.cpp:
#	echo > $@
#
#man2html_simple.o: $(srcdir)/man2html.cpp
#	-rm -f man2html_simple.cpp
#	$(LN_S) $(srcdir)/man2html.cpp man2html_simple.cpp
#	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) -DSIMPLE_MAN2HTML $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c man2html_simple.cpp
#
