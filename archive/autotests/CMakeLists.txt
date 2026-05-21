remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMAddTests)
ecm_add_test(testkioarchive.cpp TEST_NAME testkioarchive LINK_LIBRARIES KF6::KIOCore KF6::Archive Qt::Test)
