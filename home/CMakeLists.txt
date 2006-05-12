
project(kioslave-home)

add_subdirectory( kdedmodule ) 



########### next target ###############

set(libkiohome_SRCS kio_home.cpp homeimpl.cpp)

set(kio_home_PART_SRCS ${libkiohome_SRCS})

kde4_automoc(${kio_home_PART_SRCS})

kde4_add_plugin(kio_home ${kio_home_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_home )

target_link_libraries(kio_home  ${KDE4_KIO_LIBS})

install_targets(${PLUGIN_INSTALL_DIR} kio_home )


########### next target ###############


########### next target ###############

set(testhome_SRCS testhome.cpp )

kde4_automoc(${testhome_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testhome ${testhome_SRCS} ${libkiohome_SRCS})

target_link_libraries(testhome  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES home.protocol )




#original Makefile.am contents follow:

#SUBDIRS= . kdedmodule
## wizard
#
#INCLUDES = $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kio_home.la
#
#kio_home_la_SOURCES = dummy.cpp
#kio_home_la_LIBADD  = libkiohome.la $(LIB_KIO)
#kio_home_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
#
#dummy.cpp:
#	echo > dummy.cpp
#
#kde_services_DATA = home.protocol
#
#noinst_LTLIBRARIES = libkiohome.la
#libkiohome_la_SOURCES = kio_home.cpp homeimpl.cpp
#
#check_PROGRAMS = testhome
#testhome_SOURCES = testhome.cpp
#testhome_LDADD = libkiohome.la $(LIB_KIO)
#testhome_LDFLAGS = $(all_libraries)
#
### TODO in unsermake: TESTS = testhome
#check: testhome
#	./testhome
#
#messages:
#	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_home.pot
#
