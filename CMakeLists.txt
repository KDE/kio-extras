
if(NOT WIN32)
# we need a version of samba which has already smbc_set_context(), Alex
set(SAMBA_REQUIRE_SMBC_SET_CONTEXT TRUE)
set(SAMBA_REQUIRE_SMBC_OPTION_SET TRUE)
find_package(Samba)
set_package_properties(Samba PROPERTIES DESCRIPTION "the SMB client library, a version with smbc_set_context() and smbc_option_set()"
                       URL "http://www.samba.org"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SMB kioslave"
                      )
endif(NOT WIN32)

find_package(LibSSH 0.4.0)
set_package_properties(LibSSH PROPERTIES DESCRIPTION "the SSH library with SFTP support"
                       URL "http://www.libssh.org/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SFTP kioslave"
                      )

add_subdirectory( about )
# add_subdirectory( bookmarks )
# if(NOT WINCE)
# add_subdirectory( cgi )
# endif(NOT WINCE)
# add_subdirectory( filter )
# if(NOT WINCE)
# add_subdirectory( info )
# endif(NOT WINCE)
# add_subdirectory( archive )
# add_subdirectory( settings )
# add_subdirectory( applications )
# if(NOT WINCE)
# add_subdirectory( trash )
# endif(NOT WINCE)
# if(NOT WIN32)
# add_subdirectory( network )
# endif(NOT WIN32)
# add_subdirectory( remote )
# add_subdirectory( desktop )
# add_subdirectory( recentdocuments )
# add_subdirectory( fish )
if(NOT WINCE)
    add_subdirectory( thumbnail )
endif(NOT WINCE)
# add_subdirectory( docfilter )
# if (LIBSSH_FOUND)
#   add_subdirectory(sftp)
# endif (LIBSSH_FOUND)
# 
# if(NOT WIN32)
#    add_subdirectory( floppy )
#    add_subdirectory( finger )
#    add_subdirectory( man )
#    check_include_files(rpc/rpc.h HAVE_RPC_RPC_H)
#    add_feature_info("NFS kioslave" HAVE_RPC_RPC_H "The RPC library is needed to build the NFS kioslave")
#    if(HAVE_RPC_RPC_H)
#      add_subdirectory( nfs )
#    endif(HAVE_RPC_RPC_H)
# endif(NOT WIN32)
# 
# if(SAMBA_FOUND OR WIN32 AND NOT WINCE)
#     add_subdirectory(smb)
# endif(SAMBA_FOUND OR WIN32 AND NOT WINCE)

