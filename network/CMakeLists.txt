add_definitions(-DTRANSLATION_DOMAIN=\"kio5_network\")

add_definitions(
    -DQT_USE_FAST_OPERATOR_PLUS
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
)

add_subdirectory( network )
add_subdirectory( ioslave )
add_subdirectory( kded )
add_subdirectory( mimetypes )
