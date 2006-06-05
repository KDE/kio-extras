
project(kioslave-remote)

add_subdirectory( kdedmodule ) 



set(libkioremote_SRCS kio_remote.cpp remoteimpl.cpp )

########### next target ###############

set(kio_remote_PART_SRCS ${libkioremote_SRCS})

kde4_automoc(${kio_remote_PART_SRCS})

kde4_add_plugin(kio_remote ${kio_remote_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_remote )

target_link_libraries(kio_remote  ${KDE4_KIO_LIBS})

install_targets(${PLUGIN_INSTALL_DIR} kio_remote )


########### next target ###############

set(testremote_SRCS testremote.cpp ${libkioremote_SRCS} )

kde4_automoc(${testremote_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testremote ${testremote_SRCS})

target_link_libraries(testremote  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES remote.protocol )




#original Makefile.am contents follow:

#SUBDIRS= . kdedmodule
## wizard
#
#INCLUDES = $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kio_remote.la
#
#kio_remote_la_SOURCES = dummy.cpp
#kio_remote_la_LIBADD  = libkioremote.la $(LIB_KIO)
#kio_remote_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
#
#dummy.cpp:
#	echo > dummy.cpp
#
#kde_services_DATA = remote.protocol
#
#noinst_LTLIBRARIES = libkioremote.la
#libkioremote_la_SOURCES = kio_remote.cpp remoteimpl.cpp
#
#check_PROGRAMS = testremote
#testremote_SOURCES = testremote.cpp
#testremote_LDADD = libkioremote.la $(LIB_KIO)
#testremote_LDFLAGS = $(all_libraries)
#
### TODO in unsermake: TESTS = testremote
#check: testremote
#	./testremote
#
#messages:
#	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_remote.pot
#
