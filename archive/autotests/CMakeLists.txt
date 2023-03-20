remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_executable(testkioarchive testkioarchive.cpp)
target_link_libraries(testkioarchive KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::Archive Qt::Test)
ecm_mark_as_test(testkioarchive)
add_test(NAME testkioarchive COMMAND testkioarchive)
