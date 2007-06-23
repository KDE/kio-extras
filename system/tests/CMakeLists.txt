
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

set(libkiosystem_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../kio_system.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../systemimpl.cpp)

########### next target ###############
set(testsystem_SRCS testsystem.cpp )

kde4_automoc(${testsystem_SRCS})

kde4_add_test_executable(testsystem ${testsystem_SRCS} ${libkiosystem_SRCS})

target_link_libraries(testsystem  ${KDE4_KIO_LIBS} )

