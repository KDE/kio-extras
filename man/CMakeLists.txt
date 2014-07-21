add_definitions(-DTRANSLATION_DOMAIN=\"kio_man\")

add_subdirectory( tests )

########### next target ###############

set(kio_man_PART_SRCS man2html.cpp kio_man.cpp request_hash.cpp )
add_library(kio_man MODULE ${kio_man_PART_SRCS})

target_link_libraries(kio_man KF5::KIOCore Qt5::Widgets KF5::Archive KF5::KDE4Support)
install(TARGETS kio_man  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

add_library(kmanpart kmanpart.cpp)

target_link_libraries(kmanpart KF5::KHtml KF5::KDE4Support)

install(TARGETS kmanpart  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES man.protocol kmanpart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR})
