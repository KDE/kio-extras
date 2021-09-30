if(NOT BUILD_TESTING)
    return()
endif()

# allow coercing cstring to qstring, easier to write
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    smburltest.cpp
    transfertest.cpp
    shouldresumetest.cpp
    LINK_LIBRARIES
        Qt5::Test
        kio_smb_static
)
