add_subdirectory(cgi)
add_subdirectory(settings)

if(WIN32)
    add_subdirectory(finger)
    add_subdirectory(floppy)
    add_subdirectory(recentdocuments)
endif()
