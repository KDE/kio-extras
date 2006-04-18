kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/ ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### next target ###############

set(kded_medianotifier_PART_SRCS medianotifier.cpp notificationdialog.cpp ${libmediacommon_SRCS} )

kde4_add_kcfg_files(kded_medianotifier_PART_SRCS ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/mediamanagersettings.kcfgc )


kde4_automoc(${kded_medianotifier_PART_SRCS})

kde4_add_ui3_files(kded_medianotifier_PART_SRCS notificationdialogview.ui )

kde4_add_dcop_skels(kded_medianotifier_PART_SRCS medianotifier.h )

kde4_add_plugin(kded_medianotifier ${kded_medianotifier_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_medianotifier )

target_link_libraries(kded_medianotifier  ${KDE4_KIO_LIBS}  )

install_targets(${PLUGIN_INSTALL_DIR} kded_medianotifier )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES medianotifier.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../libmediacommon -I../libmediacommon $(all_includes)
#
#kde_module_LTLIBRARIES = kded_medianotifier.la
#kded_medianotifier_la_LDFLAGS = -module -avoid-version $(all_libraries)
#kded_medianotifier_la_LIBADD = ../libmediacommon/libmediacommon.la $(LIB_KDECORE) \
#                               $(LIB_KDEUI) $(LIB_KIO)
#kded_medianotifier_la_SOURCES = medianotifier.cpp medianotifier.skel \
#                                notificationdialog.cpp notificationdialogview.ui
#
#noinst_HEADERS = medianotifier.h notificationdialog.h
#
#METASOURCES = AUTO
#
#services_DATA = medianotifier.desktop
#servicesdir = $(kde_servicesdir)/kded
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kay.pot
