find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    mtpprotocoltest.cpp
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)

target_compile_definitions(mtpprotocoltest PRIVATE MTP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(mtpprotocoltest PRIVATE ../kiod_module)
