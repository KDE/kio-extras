/*
    This file is part of the smb++ library
    Copyright (C) 1999  Nicolas Brodu
    nicolas.brodu@free.fr

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

#include "defines.h"
#ifndef USE_SAMBA
/*
'#define's below come from
	Microsoft Networks/OpenNET file sharing protocol
	Intel Part Number 138446

The following values have been  assigned  for  the "core" protocol
commands.
*/

#define SMBmkdir      0x00   /* create directory */
#define SMBrmdir      0x01   /* delete directory */
#define SMBopen       0x02   /* open file */
#define SMBcreate     0x03   /* create file */
#define SMBclose      0x04   /* close file */
#define SMBflush      0x05   /* flush file */
#define SMBunlink     0x06   /* delete file */
#define SMBmv         0x07   /* rename file */
#define SMBgetatr     0x08   /* get file attributes */
#define SMBsetatr     0x09   /* set file attributes */
#define SMBread       0x0A   /* read from file */
#define SMBwrite      0x0B   /* write to file */
#define SMBlock       0x0C   /* lock byte range */
#define SMBunlock     0x0D   /* unlock byte range */
#define SMBctemp      0x0E   /* create temporary file */
#define SMBmknew      0x0F   /* make new file */
#define SMBchkpth     0x10   /* check directory path */
#define SMBexit       0x11   /* process exit */
#define SMBlseek      0x12   /* seek */
#define SMBtcon       0x70   /* tree connect */
#define SMBtdis       0x71   /* tree disconnect */
#define SMBnegprot    0x72   /* negotiate protocol */
#define SMBdskattr    0x80   /* get disk attributes */
#define SMBsearch     0x81   /* search directory */
#define SMBsplopen    0xC0   /* open print spool file */
#define SMBsplwr      0xC1   /* write to print spool file */
#define SMBsplclose   0xC2   /* close print spool file */
#define SMBsplretq    0xC3   /* return print queue */
#define SMBsends      0xD0   /* send single block message */
#define SMBsendb      0xD1   /* send broadcast message */
#define SMBfwdname    0xD2   /* forward user name */
#define SMBcancelf    0xD3   /* cancel forward */
#define SMBgetmac     0xD4   /* get machine name */
#define SMBsendstrt   0xD5   /* send start of multi-block message */
#define SMBsendend    0xD6   /* send end of multi-block message */
#define SMBsendtxt    0xD7   /* send text of multi-block message */

/*
The commands added by the LANMAN 1.0 Extended  File  Sharing
Protocol have the following command codes :
*/

#define SMBlockread      0x13   /* lock then read data */
#define SMBwriteunlock   0x14   /* write then unlock data */
#define SMBreadBraw      0x1A   /* read block raw */
#define SMBreadBmpx      0x1B   /* read block multiplexed */
#define SMBreadBs        0x1C   /* read block (secondary response) */
#define SMBwriteBraw     0x1D   /* write block raw */
#define SMBwriteBmpx     0x1E   /* write block multiplexed */
#define SMBwriteBs       0x1F   /* write block (secondary request) */
#define SMBwriteC        0x20   /* write complete response */
#define SMBsetattrE      0x22   /* set file attributes expanded */
#define SMBgetattrE      0x23   /* get file attributes expanded */
#define SMBlockingX      0x24   /* lock/unlock byte ranges and X */
#define SMBtrans         0x25   /* transaction - name, bytes in/out */
#define SMBtranss        0x26   /* transaction (secondary request/response) */
#define SMBioctl         0x27   /* IOCTL */
#define SMBioctls        0x28   /* IOCTL  (secondary request/response) */
#define SMBcopy          0x29   /* copy */
#define SMBmove          0x2A   /* move */
#define SMBecho          0x2B   /* echo */
#define SMBwriteclose    0x2C   /* Write and Close */
#define SMBopenX         0x2D   /* open and X */
#define SMBreadX         0x2E   /* read and X */
#define SMBwriteX        0x2F   /* write and X */
#define SMBsesssetup     0x73   /* Session Set Up & X (including User Logon) */
#define SMBtconX         0x75   /* tree connect and X */
#define SMBffirst        0x82   /* find first */
#define SMBfunique       0x83   /* find unique */
#define SMBfclose        0x84   /* find close */
#define SMBinvalid       0xFE   /* invalid command */

/*
The commands added by the LANMAN 1.2 Extended  File  Sharing
Protocol have the following command codes:
*/

#define SMBtrans2       0x32   /* transaction2 - function, byte in/out */
#define SMBtranss2      0x33   /* transaction2 (secondary request/response*/
#define SMBfindclose    0x34   /* find close */
#define SMBfindnclose   0x35   /* find notify close */
#define SMBuloggoffX    0x74   /* User logoff and X */


/*
Error class codes
*/

#define SUCCESS   0		// The request was successful.
#define ERRDOS 0x01		// Error is generated by the server operating system.
#define ERRSRV 0x02		// Error is generated by the server network file manager.
#define ERRHRD 0x03		// Error is an hardware error (MS-DOS int 24).
#define ERRCMD 0xFF		// Command was not in the "SMB" format.  (optional)


/*
The following error codes may be generated with the  SUCCESS
error class.
*/

//#define SUCCESS               0       // The request was successful.
#define BUFFERED           0x54       // message has been buffered
#define LOGGED             0x55       // message has been logged
#define DISPLAYED          0x56       // user message displayed


/*
The following error codes may be generated with  the  ERRDOS
error  class.   The XENIX errors equivalent to each of these
errors are noted at the end of the error description.
*/

#define ERRbadfunc   1
//             Invalid function.  The server OS did not recognize or could not  perform
//             a system call generated by the server, e.g., set the DIRECTORY attribute
//             on a data file, invalid seek mode. [EINVAL]
#define ERRbadfile   2
//             File not found.  The last component of a file's pathname  could  not  be
//             found.  [ENOENT]
#define ERRbadpath   3
//            Directory invalid.  A directory component in a  pathname  could  not  be
//               found.  [ENOENT]
#define ERRnofids    4
//             Too many open files.  The server has no file handles  (fids)  available.
//             [EMFILE]
#define ERRnoaccess  5
/*             Access denied, the requester's context does  not  permit  the  requested
               function.  This includes the following conditions.  [EPERM]
                   duplicate name errors
                   invalid rename command
                   write to fid open for read only
                   read on fid open for write only
                   attempt to open read-only file for write
                   attempt to delete read-only file
                   attempt to set attributes of a read only file
                   attempt to create a file on a full server
                   directory full
                   attempt to delete a non-empty directory
                   invalid file type (e.g., file commands on a directory)
*/
#define ERRbadfid     6
//             Invalid file handle.  The file handle specified was  not  recognized  by
//             the server.  [EBADF]
#define ERRbadmcb     7	// Memory control blocks destroyed.  [EREMOTEIO]
#define ERRnomem      8	// Insufficient server memory to perform the requested function.  [ENOMEM]
#define ERRbadmem     9	// Invalid memory block address.  [EFAULT]
#define ERRbadenv    10	// Invalid environment.  [EREMOTEIO]
#define ERRbadformat 11	// Invalid format.  [EREMOTEIO]
#define ERRbadaccess 12	// Invalid open mode.
#define ERRbaddata   13	// Invalid data (generated only by IOCTL calls within the server).  [E2BIG]
#define ERR          14	// reserved
#define ERRbaddrive  15	// Invalid drive specified.  [ENXIO]
#define ERRremcd     16
//               A Delete Directory request attempted  to  remove  the  server's  current
//               directory.  [EREMOTEIO]
#define ERRdiffdevice 17	// Not same device (e.g., a cross volume rename was attempted)  [EXDEV]
#define ERRnofiles   18
//               A File Search command can find no more files matching the specified cri-
//               teria.
#define ERRbadshare  32
//               The sharing mode specified for a non-compatibility mode  Open  conflicts
//               with existing FIDs on the file.  [ETXTBSY]
#define ERRlock      33
//               A Lock request conflicted with an existing lock or specified an  invalid
//               mode,  or  an  Unlock request attempted to remove a lock held by another
//               process.  [EDEADLOCK]
#define ERRfilexists 80
//               The file named in a Create Directory or Make New  File  request  already
//               exists.  The  error may also be generated in the Create and Rename tran-
//               sactions.  [EEXIST]




/*
The following error codes may be generated with  the  ERRSRV
error class.
*/

#define ERRerror      1
/*Non-specific error code.  It is returned under the following conditions:
                       resource other than disk space exhausted (e.g., TIDs)
                       first command on VC was not negotiate
                       multiple negotiates attempted
                       internal server error [ENFILE]
*/
#define ERRbadpw          2	// Bad password - name/password pair in a Tree Connect is invalid.
#define ERRbadtype        3	// reserved
#define ERRaccess         4
//                   The requester does not have  the  necessary  access  rights  within  the
//                   specified TID context for the requested function.  [EACCES]
#define ERRinvnid         5	// The tree ID (tid) specified in a command was invalid.
#define ERRinvnetname     6	// Invalid name supplied with tree connect.
#define ERRinvdevice      7
//                   Invalid device - printer request made to non-printer connection or  non-
//                   printer request made to printer connection.
#define ERRqfull         49	// Print queue full (files) -- returned by open print file.
#define ERRqtoobig       50	// Print queue full -- no space.
#define ERRqeof          51	// EOF on print queue dump.
#define ERRinvpfid       52	// Invalid print file FID.
#define ERRpaused        81	// Server is paused.
#define ERRmsgoff        82	// Not receiving messages.
#define ERRnoroom        83	// No room to buffer message.
#define ERRrmuns         87	// Too many remote user names.
#define ERRnosupport 0xFFFF	// Function not supported.


/*
The following error codes may be generated with  the  ERRHRD
error  class.   The XENIX errors equivalent to each of these
errors are noted at the end of the error description.
*/

#define ERRnowrite  19	// Attempt to write on write-protected diskette.  [EROFS]
#define ERRbadunit  20	// Unknown unit.  [ENODEV]
#define ERRnotready 21	// Drive not ready.  [EUCLEAN]
#define ERRbadcmd   22	// Invalid disk command.
#define ERRdata     23	// Data error (CRC).  [EIO]
#define ERRbadreq   24	// Bad request structure length.  [ERANGE]
#define ERRseek     25	// Seek error.
#define ERRbadmedia 26	// Unknown media type.
#define ERRbadsector27	// Sector not found.
#define ERRnopaper  28	// Printer out of paper.
#define ERRwrite    29	// Write fault.
#define ERRread     30	// Read fault.
#define ERRgeneral  31	// General failure.
#define ERRbadshare 32
//              A compatibility mode open conflicts with  an  existing
//              open on the file.  [ETXTBSY]

#endif
