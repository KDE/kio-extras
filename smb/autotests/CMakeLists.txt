if(NOT BUILD_TESTING)
    return()
endif()

# allow coercing cstring to qstring, easier to write
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    smbauthenticatortest.cpp
    smburltest.cpp
    transfertest.cpp
    shouldresumetest.cpp
    LINK_LIBRARIES
        Qt::Test
        kio_smb_static
)

# Copies through a Samba server that the test starts as the current user.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(SMBD_EXECUTABLE smbd PATHS /usr/sbin /usr/local/sbin)
    find_program(PDBEDIT_EXECUTABLE pdbedit PATHS /usr/bin /usr/sbin /usr/local/bin /usr/local/sbin)
    if(SMBD_EXECUTABLE AND PDBEDIT_EXECUTABLE)
        ecm_add_test(smbcopytest.cpp run_smbserver.cpp
            TEST_NAME smbcopytest
            LINK_LIBRARIES Qt::Test Qt::Network KF6::KIOCore
        )
        set_tests_properties(smbcopytest PROPERTIES ENVIRONMENT "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin")
    else()
        message(STATUS "smbd or pdbedit not found. smbcopytest skipped.")
    endif()
endif()
