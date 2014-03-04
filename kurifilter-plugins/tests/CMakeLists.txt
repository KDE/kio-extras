set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set(kurifiltertest_SRCS kurifiltertest.cpp)


kde4_add_unit_test(kurifiltertest ${kurifiltertest_SRCS})

target_link_libraries(kurifiltertest KF5::KIOWidgets Qt5::Test KF5::KDE4Support)

