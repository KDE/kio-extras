add_executable(testkioarchive testkioarchive.cpp)
target_link_libraries(testkioarchive KF5::KIOCore KF5::KDE4Support KF5::Archive Qt5::Test)
ecm_mark_as_test(testkioarchive)
add_test(testkioarchive testkioarchive)
