
# we need a version of samba which has already smbc_set_context(), Alex
set(SAMBA_REQUIRE_SMBC_SET_CONTEXT TRUE)
macro_optional_find_package(Samba)

add_subdirectory( about )
add_subdirectory( cgi )
add_subdirectory( floppy )
add_subdirectory( filter )
add_subdirectory( info )
add_subdirectory( archive )
add_subdirectory( settings )
add_subdirectory( trash )
add_subdirectory( remote )

# need kdesu
if(UNIX)
   add_subdirectory( fish )
   add_subdirectory( sftp )
endif(UNIX)

if(NOT WIN32)
   add_subdirectory( finger )
   add_subdirectory( man )
   add_subdirectory( nfs )
   add_subdirectory( thumbnail )
endif(NOT WIN32)

if(SAMBA_FOUND)
   add_subdirectory(smb)
else(SAMBA_FOUND)
macro_log_feature(SAMBA_FOUND "Samba" "Sambas libsmbclient is required for the smb-ioslave. A version which already has smbc_set_context() is required" "http://www.samba.org")
endif(SAMBA_FOUND)
