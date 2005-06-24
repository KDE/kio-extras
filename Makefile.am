if include_kioslave_ldap
LDAP_SUBDIR=ldap
endif

if include_kioslave_smb
SMB_SUBDIR=smb
endif

SUBDIRS = about cgi floppy filter fish info mac man nfs nntp pop3 smtp \
  sftp tar finger thumbnail $(LDAP_SUBDIR) $(SMB_SUBDIR) settings trash media \
  remote home system 
