## $Id$

if include_kioslave_ldap
LDAP_SUBDIR=ldap
endif

if include_kioslave_smb
SMB_SUBDIR=smb
else
SMB_SUBDIR=smbro
endif

if include_devicesdir
DEVICES_SUBDIR=devices
endif


SUBDIRS = about cgi floppy filter fish info mac man nfs nntp pop3 imap4 smtp \
  sftp tar finger thumbnail $(LDAP_SUBDIR) $(SMB_SUBDIR) $(DEVICES_SUBDIR) settings
