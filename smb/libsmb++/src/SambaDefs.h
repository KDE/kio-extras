/*
    This file is part of the smb++ library
        Copyright (C) 2000 Nicolas Brodu <nicolas.brodu@free.fr>

    It contains many definitions from several files of the Samba Unix
    SMB/Netbios implementation:
        Copyright (C) Andrew Tridgell 1992-1998
        Copyright (C) John H Terpstra 1996-1998
        Copyright (C) Luke Kenneth Casson Leighton 1996-1998
        Copyright (C) Paul Ashton 1998

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/
#ifndef SAMBADEFS_H
#define SAMBADEFS_H

#include "defines.h"
#ifdef USE_SAMBA

typedef unsigned char uchar;

#ifndef NULL
#define NULL 0
#endif
// include for the in_addr structure
#include <netinet/in.h>

typedef int BOOL;
#define False (0)
#define True (1)
#define Auto (2)

#define PSTRING_LEN 1024
#define FSTRING_LEN 128
typedef char pstring[PSTRING_LEN];
typedef char fstring[FSTRING_LEN];

const int name_type = 0x20;

#define pstrcpy(d,s) safe_strcpy((d),(s),sizeof(pstring)-1)
#define fstrcpy(d,s) safe_strcpy((d),(s),sizeof(fstring)-1)

/* deny modes */
#define DENY_DOS 0
#define DENY_ALL 1
#define DENY_WRITE 2
#define DENY_READ 3
#define DENY_NONE 4
#define DENY_FCB 7

/* share types */
#define STYPE_DISKTREE  0	/* Disk drive */
#define STYPE_PRINTQ    1	/* Spooler queue */
#define STYPE_DEVICE    2	/* Serial device */
#define STYPE_IPC       3	/* Interprocess communication (IPC) */
#define STYPE_HIDDEN    0x80000000 /* share is a hidden one (ends with $) */

/* these are used in NetServerEnum to choose what to receive */
#define SV_TYPE_WORKSTATION         0x00000001
#define SV_TYPE_SERVER              0x00000002
#define SV_TYPE_SQLSERVER           0x00000004
#define SV_TYPE_DOMAIN_CTRL         0x00000008
#define SV_TYPE_DOMAIN_BAKCTRL      0x00000010
#define SV_TYPE_TIME_SOURCE         0x00000020
#define SV_TYPE_AFP                 0x00000040
#define SV_TYPE_NOVELL              0x00000080
#define SV_TYPE_DOMAIN_MEMBER       0x00000100
#define SV_TYPE_PRINTQ_SERVER       0x00000200
#define SV_TYPE_DIALIN_SERVER       0x00000400
#define SV_TYPE_SERVER_UNIX         0x00000800
#define SV_TYPE_NT                  0x00001000
#define SV_TYPE_WFW                 0x00002000
#define SV_TYPE_SERVER_MFPN         0x00004000
#define SV_TYPE_SERVER_NT           0x00008000
#define SV_TYPE_POTENTIAL_BROWSER   0x00010000
#define SV_TYPE_BACKUP_BROWSER      0x00020000
#define SV_TYPE_MASTER_BROWSER      0x00040000
#define SV_TYPE_DOMAIN_MASTER       0x00080000
#define SV_TYPE_SERVER_OSF          0x00100000
#define SV_TYPE_SERVER_VMS          0x00200000
#define SV_TYPE_WIN95_PLUS          0x00400000
#define SV_TYPE_ALTERNATE_XPORT     0x20000000
#define SV_TYPE_LOCAL_LIST_ONLY     0x40000000
#define SV_TYPE_DOMAIN_ENUM         0x80000000
#define SV_TYPE_ALL                 0xFFFFFFFF

struct pwd_info
{
	BOOL null_pwd;
	BOOL cleartext;
	BOOL crypted;

	fstring password;

	uchar smb_lm_pwd[16];
	uchar smb_nt_pwd[16];

	uchar smb_lm_owf[24];
	uchar smb_nt_owf[128];
	size_t nt_owf_len;

	uchar lm_cli_chal[8];
	uchar nt_cli_chal[128];
	size_t nt_cli_chal_len;
};

struct ntuser_creds
{
	fstring user_name;
	fstring domain;
	struct pwd_info pwd;

	uint32 ntlmssp_flags;

};

/* DOM_CHAL - challenge info */
typedef struct chal_info
{
  uchar data[8]; /* credentials */
} DOM_CHAL;
/* 32 bit time (sec) since 01jan1970 - cifs6.txt, section 3.5, page 30 */
typedef struct time_info
{
  uint32 time;
} UTIME;

/* DOM_CREDs - timestamped client or server credentials */
typedef struct cred_info
{
  DOM_CHAL challenge; /* credentials */
  UTIME timestamp;    /* credential time-stamp */
} DOM_CRED;

struct ntdom_info
{
	unsigned char usr_sess_key[16];   /* Current user session key. */
	uint32 ntlmssp_cli_flgs;           /* ntlmssp client flags */
	uint32 ntlmssp_srv_flgs;           /* ntlmssp server flags */

	unsigned char sess_key[16];        /* Client NETLOGON session key. */
	DOM_CRED clnt_cred;                /* Client NETLOGON credential. */

	int max_recv_frag;
	int max_xmit_frag;
};

/* A netbios name structure. */
struct nmb_name {
  char         name[17];
  char         scope[64];
  unsigned int name_type;
};

struct cli_state
{
	int port;
	int fd;
	uint16 cnum;
	uint16 pid;
	uint16 mid;
	uint16 vuid;
	int protocol;
	int sec_mode;
	int rap_error;
	int privileges;

	struct ntuser_creds usr;
	BOOL retry;

	fstring eff_name;
	fstring desthost;

	/*
	 * The following strings are the
	 * ones returned by the server if
	 * the protocol > NT1.
	 */
	fstring server_type;
	fstring server_os;
	fstring server_domain;

	fstring share;
	fstring dev;
	struct nmb_name called;
	struct nmb_name calling;
	struct in_addr dest_ip;

	unsigned char cryptkey[8];
	unsigned char lm_cli_chal[8];
	unsigned char nt_cli_chal[128];
	size_t nt_cli_chal_len;

	BOOL use_ntlmv2;
	BOOL redirect;
	BOOL reuse;

	uint32 sesskey;
	int serverzone;
	uint32 servertime;
	int readbraw_supported;
	int writebraw_supported;
	int timeout;
	int max_xmit;
	int max_mux;
	char *outbuf;
	char *inbuf;
	int bufsize;
	int initialised;
	int win95;
	uint32 capabilities;

	struct ntdom_info nt;

	uint32 nt_error;                   /* NT RPC error code. */
};

#ifndef SMB_OFF_T
#  ifdef HAVE_OFF64_T
#    define SMB_OFF_T off64_t
#  else
#    define SMB_OFF_T off_t
#  endif
#endif

typedef struct file_info
{
	SMB_OFF_T size;
	uint16 mode;
	uid_t uid;
	gid_t gid;
	/* these times are normally kept in GMT */
	time_t mtime;
	time_t atime;
	time_t ctime;
	pstring name;
} file_info;

#endif
#endif // SAMBADEFS_H
