kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kcm_cgi_PART_SRCS kcmcgi.cpp )

kde4_automoc(${kcm_cgi_PART_SRCS})

kde4_add_plugin(kcm_cgi ${kcm_cgi_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_cgi )

target_link_libraries(kcm_cgi  ${KDE4_KIO_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY})

install_targets(${LIB_INSTALL_DIR}/kde4 kcm_cgi )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kcmcgi.desktop )

kde4_footer()



#original Makefile.am contents follow:

#
#kde_module_LTLIBRARIES = kcm_cgi.la
#
#kcm_cgi_la_SOURCES = kcmcgi.cpp
#kcm_cgi_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_cgi_la_LIBADD = -lkdeui $(LIB_KIO)
#
#INCLUDES= $(all_includes) 
#
#kcm_cgi_la_METASOURCES = AUTO
#
#noinst_HEADERS = kcmcgi.h
#
#xdg_apps_DATA = kcmcgi.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmcgi.pot
