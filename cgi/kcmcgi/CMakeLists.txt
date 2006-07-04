


########### next target ###############

set(kcm_cgi_PART_SRCS kcmcgi.cpp )

kde4_automoc(${kcm_cgi_PART_SRCS})

kde4_add_plugin(kcm_cgi ${kcm_cgi_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_cgi )

target_link_libraries(kcm_cgi  ${KDE4_KIO_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS kcm_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmcgi.desktop  DESTINATION  ${XDG_APPS_DIR} )




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
