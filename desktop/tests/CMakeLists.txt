set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

#include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

set(testdesktop_SRCS kio_desktop_test.cpp)

add_executable(testdesktop ${testdesktop_SRCS})
target_link_libraries(testdesktop  KF5::KIOCore KF5::Solid Qt5::Test)

ecm_mark_as_test(testdesktop)
add_test(testdesktop testdesktop)

