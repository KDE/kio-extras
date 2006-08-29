
project(kioslave-thumbnail)



macro_optional_find_package(OpenEXR)
########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp )

kde4_automoc(${kio_thumbnail_PART_SRCS})

kde4_add_plugin(kio_thumbnail ${kio_thumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_thumbnail )

target_link_libraries(kio_thumbnail  ${KDE4_KDE3SUPPORT_LIBS}  kdefx)

install(TARGETS kio_thumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp )

kde4_automoc(${imagethumbnail_PART_SRCS})

kde4_add_plugin(imagethumbnail ${imagethumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} imagethumbnail )

target_link_libraries(imagethumbnail  ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS imagethumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(svgthumbnail_PART_SRCS svgcreator.cpp )

kde4_automoc(${svgthumbnail_PART_SRCS})

kde4_add_plugin(svgthumbnail ${svgthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} svgthumbnail )

target_link_libraries(svgthumbnail ${KDE4_KDECORE_LIBS})

install(TARGETS svgthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(textthumbnail_PART_SRCS textcreator.cpp )

kde4_automoc(${textthumbnail_PART_SRCS})

kde4_add_plugin(textthumbnail ${textthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} textthumbnail )

target_link_libraries(textthumbnail  ${KDE4_KDECORE_LIBS} ${KDE4_KDEFX_LIBS} ${QT_QTGUI_LIBRARY} )

install(TARGETS textthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(htmlthumbnail_PART_SRCS htmlcreator.cpp )

kde4_automoc(${htmlthumbnail_PART_SRCS})

kde4_add_plugin(htmlthumbnail ${htmlthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} htmlthumbnail )

target_link_libraries(htmlthumbnail  ${KDE4_KDECORE_LIBS} ${KDE4_KHTML_LIBS} ${QT_QTGUI_LIBRARY})

install(TARGETS htmlthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(djvuthumbnail_PART_SRCS djvucreator.cpp )

kde4_automoc(${djvuthumbnail_PART_SRCS})

kde4_add_plugin(djvuthumbnail ${djvuthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} djvuthumbnail )

target_link_libraries(djvuthumbnail  ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS djvuthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

if(OPENEXR_FOUND)

   include_directories( ${OPENEXR_INCLUDE_DIR} )

   set(exrthumbnail_PART_SRCS exrcreator.cpp)

   kde4_automoc(${exrthumbnail_PART_SRCS})

   kde4_add_plugin(exrthumbnail ${exrthumbnail_PART_SRCS})

   kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} exrthumbnail )

   target_link_libraries(exrthumbnail  ${KDE4_KDE3SUPPORT_LIBS} ${OPENEXR_LIBRARIES})

   install(TARGETS exrthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )
   install( FILES exrthumbnail.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

endif(OPENEXR_FOUND)

########### next target ###############

if(X11_Xcursor_FOUND)

   set(cursorthumbnail_PART_SRCS cursorcreator.cpp)

   kde4_automoc(${cursorthumbnail_PART_SRCS})

   kde4_add_plugin(cursorthumbnail ${cursorthumbnail_PART_SRCS})

   kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} cursorthumbnail )

   target_link_libraries(cursorthumbnail  ${KDE4_KDE3SUPPORT_LIBS} ${X11_Xcursor_LIB})

   install(TARGETS cursorthumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )
   install( FILES cursorthumbnail.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

endif(X11_Xcursor_FOUND)


########### install files ###############

install( FILES thumbcreator.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES thumbnail.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES imagethumbnail.desktop textthumbnail.desktop                  htmlthumbnail.desktop                  djvuthumbnail.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES svgthumbnail.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/thumbnail
#
#INCLUDES = $(all_includes) $(EXR_FLAGS)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#METASOURCES = AUTO
#
#if have_xcursor
#CURSOR_MODULE = cursorthumbnail.la
#CURSOR_DATAFILE = cursorthumbnail.desktop
#endif
#
#if include_EXR_MODULES
#EXR_MODULE = exrthumbnail.la
#EXR_DATAFILE = exrthumbnail.desktop
#endif
#
#kde_module_LTLIBRARIES = kio_thumbnail.la imagethumbnail.la \
#	textthumbnail.la htmlthumbnail.la \
#	djvuthumbnail.la $(CURSOR_MODULE) \
#	$(EXR_MODULE)
#
#kio_thumbnail_la_SOURCES = thumbnail.cpp
#kio_thumbnail_la_LIBADD = $(LIB_KIO)
#kio_thumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#imagethumbnail_la_SOURCES = imagecreator.cpp
#imagethumbnail_la_LIBADD = $(LIB_KIO) # for kimageio
#imagethumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#textthumbnail_la_SOURCES = textcreator.cpp
#textthumbnail_la_LIBADD = $(LIB_KDECORE)
#textthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -lkdefx
#
#htmlthumbnail_la_SOURCES = htmlcreator.cpp
#htmlthumbnail_la_LIBADD = $(LIB_KHTML)
#htmlthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#djvuthumbnail_la_SOURCES = djvucreator.cpp
#djvuthumbnail_la_LIBADD = $(LIB_KDECORE)
#djvuthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#cursorthumbnail_la_SOURCES = cursorcreator.cpp
#cursorthumbnail_la_LIBADD = $(LIB_KDECORE) $(LIB_XCURSOR)
#cursorthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#exrthumbnail_la_SOURCES = exrcreator.cpp
#exrthumbnail_la_LIBADD = $(LIB_KDECORE) $(LIB_EXR)
#exrthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = thumbnail.h imagecreator.h textcreator.h htmlcreator.h \
#                 djvucreator.h cursorcreator.h \
#		 exrcreator.h
#
#kdelnk_DATA = thumbnail.protocol
#kdelnkdir = $(kde_servicesdir)
#
#servicetypes_DATA = thumbcreator.desktop
#servicetypesdir = $(kde_servicetypesdir)
#
#services_DATA = imagethumbnail.desktop textthumbnail.desktop \
#                htmlthumbnail.desktop \
#                djvuthumbnail.desktop \
#                $(CURSOR_DATAFILE) $(EXR_DATAFILE)
#
#servicesdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_thumbnail.pot
#
