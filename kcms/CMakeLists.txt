add_definitions(-DTRANSLATION_DOMAIN=\"kio-extras_kcms\")

if(NOT WIN32)
    add_subdirectory(trash)
endif()
add_subdirectory(webshortcuts)
add_subdirectory(smb)
add_subdirectory(proxy)
add_subdirectory(netpref)
