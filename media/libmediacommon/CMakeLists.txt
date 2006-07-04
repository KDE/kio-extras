


########### next target ###############

########### install files ###############

install( FILES mediamanagersettings.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )




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
