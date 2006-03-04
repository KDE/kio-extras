kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ksshprocesstest_SRCS ksshprocesstest.cpp )

kde4_automoc(${ksshprocesstest_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(ksshprocesstest ${ksshprocesstest_SRCS})

target_link_libraries(ksshprocesstest  ${QT_AND_KDECORE_LIBS} kdesu )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(kio_sftp_PART_SRCS 
   process.cpp 
   atomicio.cpp 
   kio_sftp.cpp 
   sftpfileattr.cpp 
   ksshprocess.cpp )

kde4_automoc(${kio_sftp_PART_SRCS})

kde4_add_plugin(kio_sftp ${kio_sftp_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_sftp )

target_link_libraries(kio_sftp  ${LIB_KDE3SUPPORT}  kdesu)

install_targets(${LIB_INSTALL_DIR}/kde4 kio_sftp )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES sftp.protocol )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/sftp
#
#INCLUDES = $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#METASOURCES = AUTO
#
######## Files
#
#check_PROGRAMS = ksshprocesstest
#
#ksshprocesstest_SOURCES = ksshprocesstest.cpp
#ksshprocesstest_LDADD = $(LIB_KSYCOCA) ksshprocess.lo process.lo atomicio.lo
#
#kde_module_LTLIBRARIES = kio_sftp.la
#
#kio_sftp_la_SOURCES = process.cpp atomicio.cpp kio_sftp.cpp sftpfileattr.cpp ksshprocess.cpp
#kio_sftp_la_LIBADD = $(LIB_KIO)
#kio_sftp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = atomicio.h kio_sftp.h ksshprocess.h process.h sftpfileattr.h sftp.h
#
#kdelnk_DATA = sftp.protocol
#kdelnkdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_sftp.pot
