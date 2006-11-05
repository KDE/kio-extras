
macro_optional_find_package(Samba)

macro_log_feature(SAMBA_HAVE_SMBC_SET_CONTEXT "Samba" "Sambas libsmbclient is required for the smb-ioslave. A version which already has smbc_set_context() is required" "http://www.samba.org")

add_subdirectory( about ) 
add_subdirectory( cgi ) 
add_subdirectory( floppy ) 
add_subdirectory( filter ) 
add_subdirectory( info ) 
add_subdirectory( man ) 
add_subdirectory( nntp ) 
add_subdirectory( pop3 ) 
add_subdirectory( smtp ) 
add_subdirectory( tar ) 
add_subdirectory( finger ) 
add_subdirectory( settings )
add_subdirectory( trash ) 
add_subdirectory( media ) 
add_subdirectory( remote ) 
add_subdirectory( home ) 
add_subdirectory( system )

# need kdesu
if(UNIX)
   add_subdirectory( fish ) 
   add_subdirectory( sftp ) 
endif(UNIX)

if(NOT WIN32)
   add_subdirectory( thumbnail )
endif(NOT WIN32)

if(NOT WIN32)
   add_subdirectory( nfs ) 
endif(NOT WIN32)

if(SAMBA_FOUND AND SAMBA_HAVE_SMBC_SET_CONTEXT)
   add_subdirectory(smb)
endif(SAMBA_FOUND AND SAMBA_HAVE_SMBC_SET_CONTEXT)

add_subdirectory(ldap)
