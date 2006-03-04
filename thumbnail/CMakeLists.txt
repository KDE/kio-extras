kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )

macro_optional_find_package(OpenEXR)
########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp )

kde4_automoc(${kio_thumbnail_PART_SRCS})

kde4_add_plugin(kio_thumbnail ${kio_thumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_thumbnail )

target_link_libraries(kio_thumbnail  ${LIB_KDE3SUPPORT}  kdefx)

install_targets(${LIB_INSTALL_DIR}/kde4 kio_thumbnail )


########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp )

kde4_automoc(${imagethumbnail_PART_SRCS})

kde4_add_plugin(imagethumbnail ${imagethumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} imagethumbnail )

target_link_libraries(imagethumbnail  ${LIB_KDE3SUPPORT})

install_targets(${LIB_INSTALL_DIR}/kde4 imagethumbnail )


########### next target ###############

set(textthumbnail_PART_SRCS textcreator.cpp )

kde4_automoc(${textthumbnail_PART_SRCS})

kde4_add_plugin(textthumbnail ${textthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} textthumbnail )

target_link_libraries(textthumbnail  ${QT_AND_KDECORE_LIBS} kdefx ${QT_QTGUI_LIBRARY} )

install_targets(${LIB_INSTALL_DIR}/kde4 textthumbnail )


########### next target ###############

set(htmlthumbnail_PART_SRCS htmlcreator.cpp )

kde4_automoc(${htmlthumbnail_PART_SRCS})

kde4_add_plugin(htmlthumbnail ${htmlthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} htmlthumbnail )

target_link_libraries(htmlthumbnail  ${QT_AND_KDECORE_LIBS} khtml ${QT_QTGUI_LIBRARY})

install_targets(${LIB_INSTALL_DIR}/kde4 htmlthumbnail )


########### next target ###############

set(djvuthumbnail_PART_SRCS djvucreator.cpp )

kde4_automoc(${djvuthumbnail_PART_SRCS})

kde4_add_plugin(djvuthumbnail ${djvuthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} djvuthumbnail )

target_link_libraries(djvuthumbnail  ${LIB_KDE3SUPPORT})

install_targets(${LIB_INSTALL_DIR}/kde4 djvuthumbnail )


########### next target ###############

if(OPENEXR_FOUND)
include_directories(${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${OPENEXR_INCLUDE_DIR})

set(exrthumbnail_PART_SRCS exrcreator.cpp)

kde4_automoc(${exrthumbnail_PART_SRCS})

kde4_add_plugin(exrthumbnail ${exrthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} exrthumbnail )

target_link_libraries(exrthumbnail  ${LIB_KDE3SUPPORT} ${OPENEXR_LIBRARIES})

install_targets(${LIB_INSTALL_DIR}/kde4 exrthumbnail )
install_files( ${SERVICES_INSTALL_DIR} FILES exrthumbnail.desktop )
endif(OPENEXR_FOUND)

########### next target ###############

if(X11_Xcursor_FOUND)
include_directories(${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${OPENEXR_INCLUDE_DIR})

set(cursorthumbnail_PART_SRCS cursorcreator.cpp)

kde4_automoc(${cursorthumbnail_PART_SRCS})

kde4_add_plugin(cursorthumbnail ${cursorthumbnail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} cursorthumbnail )

target_link_libraries(cursorthumbnail  ${LIB_KDE3SUPPORT} ${X11_Xcursor_LIB})

install_targets(${LIB_INSTALL_DIR}/kde4 cursorthumbnail )
install_files( ${SERVICES_INSTALL_DIR} FILES cursorthumbnail.desktop )
endif(X11_Xcursor_FOUND)


########### install files ###############

install_files( ${SERVICETYPES_INSTALL_DIR} FILES thumbcreator.desktop )
install_files( ${SERVICES_INSTALL_DIR} FILES thumbnail.protocol )
install_files( ${SERVICES_INSTALL_DIR} FILES imagethumbnail.desktop textthumbnail.desktop                  htmlthumbnail.desktop                  djvuthumbnail.desktop)

kde4_footer()



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
