
project(kioslave-smtp)


set(smtp_optional_includes)
set(smtp_optional_libs)

macro_optional_find_package(Sasl2)

if (SASL2_FOUND)
   set(smtp_optional_includes ${smtp_optional_includes} ${SASL2_INCLUDE_DIR})
   set(smtp_optional_libs     ${smtp_optional_libs}     ${SASL2_LIBRARIES})
   set(HAVE_LIBSASL2 1)
else (SASL2_FOUND)
   set(HAVE_LIBSASL2 0)
endif (SASL2_FOUND)

configure_file(smtp-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/smtp-config.h )

include_directories(  ${smtp_optional_includes}  )


########### next target ###############

set(kio_smtp_PART_SRCS 
   smtp.cc 
   request.cc 
   response.cc 
   capabilities.cc 
   command.cc 
   transactionstate.cc )

kde4_automoc(${kio_smtp_PART_SRCS})

kde4_add_plugin(kio_smtp ${kio_smtp_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_smtp )

target_link_libraries(kio_smtp  ${KDE4_KIO_LIBS} ${smtp_optional_libs})

install_targets(${PLUGIN_INSTALL_DIR} kio_smtp )

########### next target ###############

set(test_responseparser_SRCS test_responseparser.cc )

kde4_automoc(test_responseparser_SRCS})

if(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

kde4_add_executable(test_responseparser ${test_responseparser_SRCS})

target_link_libraries(test_responseparser  ${KDE4_KDECORE_LIBS} )

endif(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

########### next target ###############
set(test_headergeneration_SRCS test_headergeneration.cc )

kde4_automoc(test_headergeneration_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(test_headergeneration ${test_headergeneration_SRCS})

target_link_libraries(test_headergeneration  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)



########### next target ###############
set(test_commands_SRCS test_commands.cc )

kde4_automoc(test_commands_SRCS})

if(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

kde4_add_executable(test_commands ${test_commands_SRCS})

target_link_libraries(test_commands  ${KDE4_KDECORE_LIBS} )

endif(DOES_NOT_COMPILE_KDE4_BUILD_TESTS)

########### next target ###############

set(interactivesmtpserver_SRCS interactivesmtpserver.cc )

kde4_automoc(${interactivesmtpserver_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(interactivesmtpserver ${interactivesmtpserver_SRCS})

target_link_libraries(interactivesmtpserver  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES smtp.protocol smtps.protocol )




#original Makefile.am contents follow:

#
#INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)
#
#kde_module_LTLIBRARIES = kio_smtp.la
#
#kio_smtp_la_SOURCES = smtp.cc request.cc response.cc capabilities.cc command.cc transactionstate.cc
#kio_smtp_la_LIBADD  = $(LIB_KIO) $(SASL2_LIBS)
#kio_smtp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = smtp.h request.h response.h capabilities.h command.h transactionstate.h
#
#kdelnk_DATA = smtp.protocol smtps.protocol
#kdelnkdir = $(kde_servicesdir)
#
#TESTS = test_headergeneration test_responseparser test_commands
#
#check_PROGRAMS = $(TESTS) interactivesmtpserver
#
#test_headergeneration_SOURCES = test_headergeneration.cc
#test_headergeneration_LDADD =  $(LIB_KDECORE)
#test_headergeneration_LDFLAGS = $(all_libraries)
#
#test_responseparser_SOURCES = test_responseparser.cc
#test_responseparser_LDADD =  $(LIB_KDECORE)
#test_responseparser_LDFLAGS = $(all_libraries)
#
#test_commands_SOURCES = test_commands.cc
#test_commands_LDADD =  $(kio_smtp_la_LIBADD)
#test_commands_LDFLAGS = $(all_libraries)
#
#interactivesmtpserver_SOURCES = interactivesmtpserver.cc
#interactivesmtpserver_LDADD = $(LIB_QT)
#interactivesmtpserver_LDFLAGS = $(all_libraries)
#interactivesmtpserver_METASOURCES = AUTO
#
#messages:
#	$(XGETTEXT) *.cc -o $(podir)/kio_smtp.pot
