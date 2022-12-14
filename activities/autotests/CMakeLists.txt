if(NOT BUILD_TESTING)
    return()
endif()

# allow coercing cstring to qstring, easier to write
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

ecm_add_tests(
    ActivityBasicTest.cpp
    LINK_LIBRARIES
    Qt::Test
    kio_activities_a
)
