# SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
# SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import paramiko"
        RESULT_VARIABLE PARAMIKO_FOUND
        ERROR_QUIET
    )
    if(PARAMIKO_FOUND EQUAL 0)
        find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sftpserver
                DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

        add_library(kio_sftp_test_mock SHARED mock_getpwuid_r.cpp)
        target_link_libraries(kio_sftp_test_mock PRIVATE ${CMAKE_DL_LIBS})
        set_target_properties(kio_sftp_test_mock PROPERTIES CXX_VISIBILITY_PRESET default)

        include(ECMAddTests)
        ecm_add_test(test_sftp_resume.cpp run_sftpserver.cpp
                TEST_NAME test_sftp_resume
                LINK_LIBRARIES Qt::Test KF6::KIOCore
        )

        ecm_add_test(test_sftp_operations.cpp run_sftpserver.cpp
                TEST_NAME test_sftp_operations
                LINK_LIBRARIES Qt::Test Qt::Gui KF6::KIOCore
        )

        # The mock library must be LD_PRELOADed so libssh uses the fake known_hosts.
        # If building with ASan, its runtime must come first; find it via the compiler.
        set(MOCK_LIB_PATH "${CMAKE_BINARY_DIR}/bin/libkio_sftp_test_mock.so")
        set(LD_PRELOAD_VALUE "${MOCK_LIB_PATH}")
        if(DEFINED ENV{LD_PRELOAD})
            set(LD_PRELOAD_VALUE "$ENV{LD_PRELOAD}:${MOCK_LIB_PATH}")
        elseif(CMAKE_CXX_FLAGS MATCHES "-fsanitize=address")
            execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libasan.so
                OUTPUT_VARIABLE ASAN_LIB
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(ASAN_LIB AND NOT ASAN_LIB STREQUAL "libasan.so" AND EXISTS "${ASAN_LIB}")
                set(LD_PRELOAD_VALUE "${ASAN_LIB}:${MOCK_LIB_PATH}")
            endif()
        endif()
        set_tests_properties(test_sftp_resume test_sftp_operations
            PROPERTIES ENVIRONMENT "LD_PRELOAD=${LD_PRELOAD_VALUE};QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin"
        )
    else()
        message(STATUS "paramiko not found. SFTP tests skipped.")
    endif()

endif()
