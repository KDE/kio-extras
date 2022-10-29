# allow coercing cstring to qstring, easier to write
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    thumbnailtest.cpp
    LINK_LIBRARIES
        Qt::Test
        KF5::KIOWidgets
)
