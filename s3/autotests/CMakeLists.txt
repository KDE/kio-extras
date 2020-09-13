include(ECMAddTests)

ecm_add_test(
    s3urltest.cpp ../s3url.cpp
    LINK_LIBRARIES Qt5::Test
    TEST_NAME s3urltest)
