set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
kde4_add_unit_test(testkioarchive testkioarchive.cpp)
target_link_libraries(testkioarchive KF5::KIOCore ${QT_QTTEST_LIBRARY})
