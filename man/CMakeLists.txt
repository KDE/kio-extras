add_definitions(-DTRANSLATION_DOMAIN=\"kio5_man\")

find_package(KF5 REQUIRED COMPONENTS Codecs)

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

########### next target ###############

set(kio_man_PART_SRCS man2html.cpp kio_man.cpp request_hash.cpp kio_man_debug.cpp)
ecm_gperf_generate(requests.gperf ${CMAKE_CURRENT_BINARY_DIR}/request_gperf.h kio_man_PART_SRCS)
add_library(kio_man MODULE ${kio_man_PART_SRCS})

target_link_libraries(kio_man KF5::KIOCore Qt5::Widgets KF5::Archive KF5::I18n KF5::Codecs Qt5::Network)
set_target_properties(kio_man PROPERTIES OUTPUT_NAME "man")
install(TARGETS kio_man DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### next target ###############

add_library(kmanpart kmanpart.cpp )
target_compile_definitions(kmanpart PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(kmanpart KF5::KHtml)

install(TARGETS kmanpart  DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install(FILES man.protocol kmanpart.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})
