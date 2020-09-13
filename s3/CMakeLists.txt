add_definitions(-DTRANSLATION_DOMAIN=\"kio5_s3\")

set(kio_s3_SRCS kio_s3.cpp s3url.cpp)

ecm_qt_declare_logging_category(kio_s3_SRCS
    HEADER s3debug.h
    IDENTIFIER S3
    CATEGORY_NAME kf5.kio.s3)

kcoreaddons_add_plugin(kio_s3
    SOURCES ${kio_s3_SRCS}
    JSON s3.json
    INSTALL_NAMESPACE kf5/kio)

target_link_libraries(kio_s3
    Qt5::Core
    Qt5::Network
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    ${AWSSDK_LINK_LIBRARIES})

set_target_properties(kio_s3 PROPERTIES OUTPUT_NAME "s3")

add_subdirectory(autotests)
