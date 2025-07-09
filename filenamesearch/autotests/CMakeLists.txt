# allow coercing cstring to qstring, easier to write
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    filenamesearchtest.cpp
    filenamesearchexternaltest.cpp
    LINK_LIBRARIES
        Qt::Test
        KF6::KIOWidgets
)
