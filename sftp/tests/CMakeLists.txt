add_definitions (-DQT3_SUPPORT)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

########### next target ###############
set(ksshprocesstest_SRCS
    ksshprocesstest.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../ksshprocess.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../process.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../atomicio.cpp )

kde4_automoc(${ksshprocesstest_SRCS})

kde4_add_test_executable(ksshprocesstest ${ksshprocesstest_SRCS})

target_link_libraries(ksshprocesstest  ${KDE4_KDECORE_LIBS} ${KDE4_KDESU_LIBS} )


