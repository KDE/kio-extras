
project(kioslave-system)

add_subdirectory( kdedmodule ) 
add_subdirectory( entries ) 
add_subdirectory( mimetypes ) 



########### next target ###############
set(libkiosystem_SRCS kio_system.cpp systemimpl.cpp)
set(kio_system_PART_SRCS ${libkiosystem_SRCS})

kde4_automoc(${kio_system_PART_SRCS})

kde4_add_plugin(kio_system ${kio_system_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_system )

target_link_libraries(kio_system  ${KDE4_KIO_LIBS})

install(TARGETS kio_system  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############


########### next target ###############

set(testsystem_SRCS testsystem.cpp )

kde4_automoc(${testsystem_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testsystem ${testsystem_SRCS} ${libkiosystem_SRCS})

target_link_libraries(testsystem  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES system.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#SUBDIRS= . kdedmodule entries mimetypes
#
#INCLUDES = $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kio_system.la
#
#kio_system_la_SOURCES = dummy.cpp
#kio_system_la_LIBADD  = libkiosystem.la $(LIB_KIO)
#kio_system_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
#
#dummy.cpp:
#	echo > dummy.cpp
#
#kde_services_DATA = system.protocol
#
#noinst_LTLIBRARIES = libkiosystem.la
#libkiosystem_la_SOURCES = kio_system.cpp systemimpl.cpp
#
#check_PROGRAMS = testsystem
#testsystem_SOURCES = testsystem.cpp
#testsystem_LDADD = libkiosystem.la $(LIB_KIO)
#testsystem_LDFLAGS = $(all_libraries)
#
### TODO in unsermake: TESTS = testsystem
#check: testsystem
#	./testsystem
#
#messages:
#	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_system.pot
#
