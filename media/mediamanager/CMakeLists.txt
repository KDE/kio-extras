kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}   ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/ )


########### next target ###############
kde4_add_kcfg_files(libmediacommon_SRCS ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/mediamanagersettings.kcfgc )

set(kded_mediamanager_PART_SRCS ${libmediacommon_SRCS}
   mediamanager.cpp 
   medialist.cpp 
   backendbase.cpp 
   fstabbackend.cpp 
   removablebackend.cpp 
   mediadirnotify.cpp )

kde4_automoc(${kded_mediamanager_PART_SRCS})

kde4_add_dcop_skels(kded_mediamanager_PART_SRCS mediamanager.h mediadirnotify.h )

kde4_add_plugin(kded_mediamanager ${kded_mediamanager_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_mediamanager )

target_link_libraries(kded_mediamanager  ${QT_AND_KDECORE_LIBS} mediacommon )

install_targets(${LIB_INSTALL_DIR}/kde4 kded_mediamanager )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES mediamanager.desktop )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_mediamanager.la
#
#if include_media_halbackend
#HALBACKEND_INCS = $(HAL_CFLAGS) $(DBUS_CFLAGS) $(QTDBUS_CFLAGS)
#endif
#
#METASOURCES = AUTO
#INCLUDES = -I$(srcdir)/../libmediacommon -I../libmediacommon $(HALBACKEND_INCS) $(all_includes)
#
#if include_media_halbackend
#HALBACKEND_LIB = libhalbackend.la
#libhalbackend_la_SOURCES = halbackend.cpp
#libhalbackend_la_LDFLAGS = -avoid-version $(all_libraries) -no-undefined
#libhalbackend_la_LIBADD = $(HAL_LIBS) $(DBUS_LIBS) $(QTDBUS_LIBS)
#endif
#
#if include_media_linuxcdpolling
#LINUXCDPOLLING_LIB = liblinuxcdpolling.la
#liblinuxcdpolling_la_SOURCES = linuxcdpolling.cpp
#liblinuxcdpolling_la_LDFLAGS = -avoid-version $(all_libraries) -no-undefined
#endif
#
#noinst_LTLIBRARIES = $(LINUXCDPOLLING_LIB) $(HALBACKEND_LIB)
#
#kded_mediamanager_la_SOURCES = mediamanager.cpp mediamanager.skel medialist.cpp backendbase.cpp fstabbackend.cpp removablebackend.cpp mediadirnotify.cpp mediadirnotify.skel
#kded_mediamanager_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_mediamanager_la_LIBADD = $(LIB_KSYCOCA) ../libmediacommon/libmediacommon.la $(HALBACKEND_LIB) $(LINUXCDPOLLING_LIB)
#
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = mediamanager.desktop
#
