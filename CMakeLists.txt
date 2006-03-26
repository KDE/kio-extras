kde4_header()

macro_optional_find_package(Samba)
macro_optional_find_package(Ldap)

add_subdirectory( about ) 
add_subdirectory( cgi ) 
add_subdirectory( floppy ) 
add_subdirectory( filter ) 
###### READD IT !!!!!!
#add_subdirectory( fish ) 
add_subdirectory( info ) 
add_subdirectory( mac ) 
add_subdirectory( man ) 
add_subdirectory( nfs ) 
add_subdirectory( nntp ) 
add_subdirectory( pop3 ) 
add_subdirectory( smtp ) 
# need kdesu
if(UNIX)
	add_subdirectory( sftp ) 
endif(UNIX)
add_subdirectory( tar ) 
add_subdirectory( finger ) 
add_subdirectory( thumbnail ) 
add_subdirectory( settings ) 
add_subdirectory( trash ) 
#add_subdirectory( media ) 
add_subdirectory( remote ) 
add_subdirectory( home ) 
add_subdirectory( system )
# Test better (see configure.in.in)
if(SAMBA_FOUND)
	add_subdirectory(smb)
endif(SAMBA_FOUND)

if(LDAP_FOUND)
	add_subdirectory(ldap)
endif(LDAP_FOUND)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#if include_kioslave_ldap
#LDAP_SUBDIR=ldap
#endif
#
#if include_kioslave_smb
#SMB_SUBDIR=smb
#endif
#
#SUBDIRS = about cgi floppy filter fish info mac man nfs nntp pop3 smtp \
#  sftp tar finger thumbnail $(LDAP_SUBDIR) $(SMB_SUBDIR) settings trash media \
#  remote home system
