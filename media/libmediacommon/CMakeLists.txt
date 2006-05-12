kde4_header()



########### next target ###############

########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES mediamanagersettings.kcfg )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = $(all_includes)
#METASOURCES = AUTO
#
#noinst_LTLIBRARIES = libmediacommon.la
#libmediacommon_la_SOURCES = medium.cpp actionlistboxitem.cpp \
#                            notifieraction.cpp notifierserviceaction.cpp \
#                            notifiernothingaction.cpp notifieropenaction.cpp \
#                            notifiersettings.cpp mediamanagersettings.kcfgc
#
#kde_kcfg_DATA = mediamanagersettings.kcfg
#
