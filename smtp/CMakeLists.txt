kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


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

target_link_libraries(kio_smtp  ${QT_AND_KDECORE_LIBS} kio )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_smtp )


########### next target ###############

set($(TESTS)_SRCS dummy.cpp )

kde4_automoc(${$(TESTS)_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable($(TESTS) ${$(TESTS)_SRCS})

target_link_libraries($(TESTS)  ${QT_AND_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(interactivesmtpserver_SRCS interactivesmtpserver.cc )

kde4_automoc(${interactivesmtpserver_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(interactivesmtpserver ${interactivesmtpserver_SRCS})

target_link_libraries(interactivesmtpserver  ${QT_AND_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES smtp.protocol smtps.protocol )

kde4_footer()



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
