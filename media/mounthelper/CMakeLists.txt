kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/ ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### next target ###############

set(kio_media_mounthelper_SRCS kio_media_mounthelper.cpp ${libmediacommon_SRCS} )

kde4_automoc(${kio_media_mounthelper_SRCS})

kde4_add_executable(kio_media_mounthelper ${kio_media_mounthelper_SRCS})

target_link_libraries(kio_media_mounthelper  ${KDE4_KIO_LIBS} )

install(TARGETS kio_media_mounthelper DESTINATION bin)


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#bin_PROGRAMS = kio_media_mounthelper
#
#INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)
#AM_LDFLAGS = $(all_libraries)
#
#kio_media_mounthelper_SOURCES = kio_media_mounthelper.cpp
#
#kio_media_mounthelper_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#kio_media_mounthelper_LDADD = $(LIB_KIO) ../libmediacommon/libmediacommon.la
#
#METASOURCES = AUTO
#
