

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../ )

set(libkiohome_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../kio_home.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../homeimpl.cpp)

########### next target ###############
set(testhome_SRCS testhome.cpp )

kde4_automoc(${testhome_SRCS})
kde4_automoc(${libkiohome_SRCS})

kde4_add_test_executable(testhome ${testhome_SRCS} ${libkiohome_SRCS})

target_link_libraries(testhome  ${KDE4_KIO_LIBS} )
