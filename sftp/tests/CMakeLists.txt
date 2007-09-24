set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
add_definitions (-DQT3_SUPPORT)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

########### next target ###############
set(ksshprocesstest_SRCS
    ksshprocesstest.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../ksshprocess.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../process.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../atomicio.cpp )


kde4_add_executable(ksshprocesstest TEST ${ksshprocesstest_SRCS})

target_link_libraries(ksshprocesstest ${KDE4_KDESU_LIBS} )


