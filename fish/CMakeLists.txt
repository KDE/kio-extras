
project(kioslave-fish)



########### next target ###############

# on Linux there is md5sum, on FreeBSD there is md5
find_program(MD5SUM_EXECUTABLE NAMES md5sum md5 )

if (MD5SUM_EXECUTABLE)
 
   if ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
      set(CUT_ARG "-f 1")                            # for md5sum the sum is in the 1st column
   else ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
      set(CUT_ARG "-f 4")                            # for md5 the sum is in the 4th column
   endif ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
 
   add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h 
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_fishcode.sh ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl ${MD5SUM_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h "${CUT_ARG}"
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl )
   
   
   set(kio_fish_PART_SRCS fish.cpp ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h)
   
   kde4_automoc(kio_fish ${kio_fish_PART_SRCS})
   
   kde4_add_plugin(kio_fish ${kio_fish_PART_SRCS})
   
   kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_fish )
   
   target_link_libraries(kio_fish ${KDE4_KIO_LIBS})
   
install(TARGETS kio_fish  DESTINATION ${PLUGIN_INSTALL_DIR} )
   
   
   ########### install files ###############
   
install( FILES fish.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
   

endif (MD5SUM_EXECUTABLE)





#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kio_fish.la
#
#INCLUDES = $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#kio_fish_la_SOURCES = fish.cpp
#kio_fish_la_LIBADD = $(LIB_KSYCOCA) #$(LIBUTIL) 
#kio_fish_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = fishcode.h fish.h
#
#EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO FAQ fish.pl
#
#DISTCLEANFILES = fishcode.h
#
#kdelnk_DATA = fish.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#fish.lo: fishcode.h
#
#fishcode.h: fish.pl
#	SUM=`$(MD5SUM) $(srcdir)/fish.pl | cut -d ' ' $(MD5SUM_CUT)`; \
#	echo '#define CHECKSUM "'$$SUM'"' > $@; \
#	echo 'static const char *fishCode(' >> $@; \
#	sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$$/\\n"/;/^"\\n"$$/d;s/{CHECKSUM}/'$$SUM'/;' $(srcdir)/fish.pl >> $@; \
#	echo ');' >> $@;
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_fish.pot
#
#
#
