add_definitions(-DTRANSLATION_DOMAIN=\"kio5_fish\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(CheckIncludeFiles)
include(CheckFunctionExists)

check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(util.h HAVE_UTIL_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(pty.h HAVE_PTY_H)
check_include_files("sys/types.h;libutil.h" HAVE_LIBUTIL_H)
check_function_exists(getpt       HAVE_GETPT)
check_function_exists(grantpt     HAVE_GRANTPT)
include(CheckLibraryExists)
check_library_exists(util  openpty "" HAVE_OPENPTY)
check_library_exists(util  isastream "" HAVE_ISASTREAM)

find_library(UTIL_LIBRARIES util)
mark_as_advanced(UTIL_LIBRARIES)

configure_file(config-fish.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-fish.h)

########### next target ###############
# on Linux there is md5sum, on FreeBSD there is md5
find_program(MD5SUM_EXECUTABLE NAMES md5sum md5 )
    
if (MD5SUM_EXECUTABLE)

   add_library(kio_fish MODULE)
   set_target_properties(kio_fish PROPERTIES
      OUTPUT_NAME "fish"
   )

   if ("${MD5SUM_EXECUTABLE}" MATCHES "md5sum")
      set(CUT_ARG "-f 1")                            # for md5sum the sum is in the 1st column
   else ()
      set(CUT_ARG "-f 4")                            # for md5 the sum is in the 4th column
   endif ()

   if (WIN32)
      set(FISH_GENERATOR "generate_fishcode.bat")
   else ()
      set(FISH_GENERATOR "generate_fishcode.sh")
   endif ()
   set(fishcode_HDR "${CMAKE_CURRENT_BINARY_DIR}/fishcode.h")
   add_custom_command(OUTPUT ${fishcode_HDR}
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${FISH_GENERATOR} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl ${MD5SUM_EXECUTABLE} ${fishcode_HDR} "${CUT_ARG}"
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
   set_source_files_properties(${fishcode_HDR} PROPERTIES SKIP_AUTOMOC TRUE)

   target_sources(kio_fish PRIVATE
      fish.cpp
      ${fishcode_HDR}
   )

   ecm_qt_declare_logging_category(kio_fish
      HEADER loggingcategory.h
      IDENTIFIER KIO_FISH_DEBUG
      CATEGORY_NAME kf.kio.slaves.fish
      OLD_CATEGORY_NAMES kio_fish
      DEFAULT_SEVERITY Info
      DESCRIPTION "KIO fish"
      EXPORT KIO_EXTRAS
   )

   target_link_libraries(kio_fish KF5::KIOCore KF5::WidgetsAddons KF5::I18n Qt5::Network)

   if (UTIL_LIBRARIES)
      target_link_libraries(kio_fish ${UTIL_LIBRARIES})
   endif ()

   install(TARGETS kio_fish  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio )

endif ()
