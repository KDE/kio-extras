kde4_header()

project(kioslave-ldap)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kio_ldap_PART_SRCS kio_ldap.cpp )

kde4_automoc(${kio_ldap_PART_SRCS})

kde4_add_plugin(kio_ldap ${kio_ldap_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_ldap )

target_link_libraries(kio_ldap  ${KDE4_KIO_LIBS} kabc ${LDAP_LIBRARIES} ${LBER_LIBRARIES} )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_ldap )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES ldap.protocol ldaps.protocol )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/ldap
#
#INCLUDES   = $(all_includes) $(LDAP_INCS)
#AM_CXXFLAGS = -DLDAP_DEPRECATED
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LDAP_RPATH)
#LDADD      = $(LIB_KIO) $(LDAP_LIBS)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_ldap.la
#
#kio_ldap_la_SOURCES = kio_ldap.cpp
#kio_ldap_la_LIBADD = $(LIB_KIO) $(LDAP_LIBS) $(LIB_KABC)
#kio_ldap_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LDAP_RPATH) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = kio_ldap.h
#
#kdelnk_DATA = ldap.protocol ldaps.protocol
#kdelnkdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_ldap.pot
