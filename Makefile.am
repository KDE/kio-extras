## $Id$

if include_kioslave_ldap
LDAP_SUBDIR=ldap
endif

if include_kioslave_smb
SMB_SUBDIR=smb
endif

if include_devicesdir
DEVICES_SUBDIR=devices
endif


SUBDIRS = cgi floppy filter fish info mac man nfs nntp pop3 imap4 smtp \
  smbro sftp tar zip finger thumbnail $(LDAP_SUBDIR) $(SMB_SUBDIR) $(DEVICES_SUBDIR)
