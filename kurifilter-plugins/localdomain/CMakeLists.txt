check_library_exists(nsl gethostbyname "" HAVE_NSL_LIBRARY)

set(localdomainurifilter_PART_SRCS localdomainurifilter.cpp )

kde4_add_plugin(localdomainurifilter ${localdomainurifilter_PART_SRCS})

target_link_libraries(localdomainurifilter ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS})
if (HAVE_NSL_LIBRARY)
target_link_libraries(localdomainurifilter nsl)
endif (HAVE_NSL_LIBRARY)

install(TARGETS localdomainurifilter DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(klocaldomainurifilterhelper_SRCS klocaldomainurifilterhelper.c)

kde4_add_executable(klocaldomainurifilterhelper NOGUI ${klocaldomainurifilterhelper_SRCS})

target_link_libraries(klocaldomainurifilterhelper ${KDE4_KDECORE_LIBS})
if (HAVE_NSL_LIBRARY)
target_link_libraries(klocaldomainurifilterhelper nsl)
endif (HAVE_NSL_LIBRARY)


install(TARGETS klocaldomainurifilterhelper DESTINATION ${LIBEXEC_INSTALL_DIR})

########### install files ###############

install(FILES localdomainurifilter.desktop DESTINATION ${SERVICES_INSTALL_DIR})
