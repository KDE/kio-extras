remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_executable(testkioarchive testkioarchive.cpp)
target_link_libraries(testkioarchive KF6::KIOCore KF6::Archive Qt::Test)
ecm_mark_as_test(testkioarchive)
add_test(NAME testkioarchive COMMAND testkioarchive)
