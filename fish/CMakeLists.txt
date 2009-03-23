check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(util.h HAVE_UTIL_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(pty.h HAVE_PTY_H)
check_include_files("sys/types.h;libutil.h" HAVE_LIBUTIL_H)
check_function_exists(getpt       HAVE_GETPT)
check_function_exists(grantpt     HAVE_GRANTPT)
check_library_exists(util  openpty "" HAVE_OPENPTY)
check_library_exists(util  isastream "" HAVE_ISASTREAM)

find_library(UTIL_LIBRARIES util)
mark_as_advanced(UTIL_LIBRARIES)

configure_file(config-fish.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-fish.h)

########### next target ###############
# on Linux there is md5sum, on FreeBSD there is md5
find_program(MD5SUM_EXECUTABLE NAMES md5sum md5 )
    
if (MD5SUM_EXECUTABLE)

   if ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
      set(CUT_ARG "-f 1")                            # for md5sum the sum is in the 1st column
   else ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
      set(CUT_ARG "-f 4")                            # for md5 the sum is in the 4th column
   endif ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")

   if (WIN32)
      set(FISH_GENERATOR "generate_fishcode.bat")
   else (WIN32)
      set(FISH_GENERATOR "generate_fishcode.sh")
   endif (WIN32)
   add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h 
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${FISH_GENERATOR} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl ${MD5SUM_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h "${CUT_ARG}"
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

   set(kio_fish_PART_SRCS fish.cpp ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h)

   kde4_add_plugin(kio_fish ${kio_fish_PART_SRCS})

   target_link_libraries(kio_fish ${KDE4_KIO_LIBS})

   if (UTIL_LIBRARIES)
      target_link_libraries(kio_fish ${UTIL_LIBRARIES})
   endif (UTIL_LIBRARIES)

   install(TARGETS kio_fish  DESTINATION ${PLUGIN_INSTALL_DIR} )


   ########### install files ###############

   install( FILES fish.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

endif (MD5SUM_EXECUTABLE OR WIN32)
