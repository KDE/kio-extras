kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/)


########### next target ###############

set(kcm_media_PART_SRCS 
   notifiermodule.cpp 
   serviceconfigdialog.cpp 
   mimetypelistboxitem.cpp 
   managermodule.cpp 
   main.cpp ${libmediacommon_SRCS})

kde4_automoc(${kcm_media_PART_SRCS})

kde4_add_ui3_files(kcm_media_PART_SRCS notifiermoduleview.ui serviceview.ui managermoduleview.ui )

kde4_add_plugin(kcm_media ${kcm_media_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_media )

target_link_libraries(kcm_media  ${QT_AND_KDECORE_LIBS} mediacommon kio )

install_targets(${LIB_INSTALL_DIR}/kde4 kcm_media )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES media.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../libmediacommon -I../libmediacommon $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kcm_media.la
#kcm_media_la_SOURCES = notifiermodule.cpp notifiermoduleview.ui \
#                       serviceconfigdialog.cpp serviceview.ui \
#                       mimetypelistboxitem.cpp \
#                       managermodule.cpp managermoduleview.ui \
#                       main.cpp
#
#kcm_media_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#kcm_media_la_LIBADD = ../libmediacommon/libmediacommon.la $(LIB_KIO)
#
#noinst_HEADERS = notifiermodule.h managermodule.h \
#                 serviceconfigdialog.h mimetypelistboxitem.h \
#                 main.h
#
#xdg_apps_DATA = media.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmmedia.pot
