kde4_header()

add_subdirectory( kdedmodule ) 
add_subdirectory( entries ) 
add_subdirectory( mimetypes ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kio_system_PART_SRCS dummy.cpp )

kde4_automoc(${kio_system_PART_SRCS})

kde4_add_plugin(kio_system ${kio_system_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_system )

target_link_libraries(kio_system  ${QT_AND_KDECORE_LIBS} kiosystem kio )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_system )


########### next target ###############


########### next target ###############

set(testsystem_SRCS testsystem.cpp )

kde4_automoc(${testsystem_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testsystem ${testsystem_SRCS})

target_link_libraries(testsystem  ${QT_AND_KDECORE_LIBS} kiosystem kio )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES system.protocol )

kde4_footer()



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
