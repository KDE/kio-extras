########### kio_applications ###############

kde4_add_plugin(kio_applications kio_applications.cpp)

target_link_libraries(kio_applications  ${KDE4_KIO_LIBS} )
if(WINCE)
target_link_libraries(kio_applications ${WCECOMPAT_LIBRARIES})
endif(WINCE)

install(TARGETS kio_applications  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  programs.protocol applications.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
