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

#ifndef __SMBPACKET_H__
#define __SMBPACKET_H__
#include "defines.h"
#ifndef USE_SAMBA

#include <sys/stat.h>
#include "SessionPacket.h"

class SMBPacket : public SessionMessagePacket
{
protected:
	uint8 dialect;	// protocol to be used

//	uint8   smb_idf[4];   /* contains 0xFF, 'SMB' */
	uint8   smb_com;      /* command code */
	uint8   smb_rcls;     /* error code class */
//	uint8   smb_reh;      /* reserved (contains AH if DOS INT-24 ERR) */
	uint16  smb_err;      /* error code */
//	uint8   smb_reb;      /* reserved */
//	uint16  smb_res[7];   /* reserved */
	uint16  smb_tid;      /* tree id #  */
	uint16  smb_pid;      /* caller's process id # */
	uint16  smb_uid;      /* user id # */
	uint16  smb_mid;      /* mutiplex id #  */
	uint8   smb_wct;      /* count of parameter words */
	uint16  *smb_vwv;     /* variable # words of params */
	uint16  smb_bcc;      /* # bytes of data following */
	uint8   *smb_buf;     /* data bytes */
	
	void update();	// parent virtual

public:
	SMBPacket(uint8 dial=0);
	~SMBPacket();
// Parent virtuals
	uint16 getDataLength(); // Get the real length of the data
	uint8 *getData();
// those 2 functions update() if necessary before looking up
	uint8 getSMBType();
	uint16 getError();
// converts data field, return -1 on error
	int parse(SessionPacket *p);
};



/*
There follow all the command packets
I do not mean to code them all for the moment,
as some are beyond the scope of this library
For example, SMBsendsPacket should be used to send
a "message" packet, i.e. a winpopup I suppose.

But maybe one day...

*/

class SMBmkdirPacket : public SMBPacket    /* create directory */
{
public:
	SMBmkdirPacket(uint16 TID=0, const char *pathname="", uint8 dial=0);
};

class SMBrmdirPacket : public SMBmkdirPacket    /* delete directory */
{
public:
	SMBrmdirPacket(uint16 TID=0, const char *pathname="", uint8 dial=0);
};

class SMBopenPacket : public SMBPacket     /* open file */
{
public:
	SMBopenPacket(uint16 TID=0, const char *filename="", int flags=2, uint8 dial=0);
	uint16 getFID();
};

class SMBcreatePacket : public SMBPacket   /* create file */
{
public:
	SMBcreatePacket(uint16 TID=0, const char *filename="", uint8 dial=0);
	uint16 getFID();
};

class SMBclosePacket : public SMBPacket    /* close file */
{
public:
	SMBclosePacket(uint16 fid=0);
};

class SMBflushPacket : public SMBPacket    /* flush file */
{
};

class SMBunlinkPacket : public SMBPacket   /* delete file */
{
public:
	SMBunlinkPacket(uint16 TID=0, const char *filename="", uint8 dial=0);
};

class SMBmvPacket : public SMBPacket       /* rename file */
{
public:
	SMBmvPacket(uint16 TID=0, const char *srcfile="", const char *dstfile="", uint8 dial=0);
};

class SMBgetatrPacket : public SMBPacket   /* get file attributes */
{
public:
	SMBgetatrPacket(uint16 tid=0, const char *filename="", uint8 dial=0);
	struct stat *getStat();
};

class SMBsetatrPacket : public SMBPacket   /* set file attributes */
{
};

class SMBreadPacket : public SMBPacket     /* read from file */
{
public:
	SMBreadPacket(uint16 tid=0, uint16 fid=0, uint32 offset=0, uint16 count=0, uint8 dial=0);
	uint16 getReadCount();
	uint8 *getReadData();
};

class SMBwritePacket : public SMBPacket    /* write to file */
{
public:
	SMBwritePacket(uint16 tid=0, uint16 fid=0, uint32 offset=0, uint8* buf=0, uint16 length=0, uint8 dial=0);
	int32 getWrittenCount();
};

class SMBlockPacket : public SMBPacket     /* lock byte range */
{
};

class SMBunlockPacket : public SMBPacket   /* unlock byte range */
{
};

class SMBctempPacket : public SMBPacket    /* create temporary file */
{
};

class SMBmknewPacket : public SMBPacket    /* make new file */
{
};

class SMBchkpthPacket : public SMBPacket   /* check directory path */
{
};

class SMBexitPacket : public SMBPacket     /* process exit */
{
};

class SMBlseekPacket : public SMBPacket    /* seek */
{
};

class SMBtconPacket : public SMBPacket     /* tree connect */
{
protected:
	char* path;
	uint8* password;
	uint32 passLen;
	char* device;
	void update();	// Parent virtual
public:
	SMBtconPacket(const char* chemin=0, const uint8* pass=0, uint32 length=0, const char* dev="A:", uint8 dial=0);
	void setPath(const char* chemin);
	void setPassword(const uint8* pass, uint32 length);
	void setDevice(const char* dev);
	virtual uint16 getMaxMessageSize(); // returns 0 on error
	virtual uint16 getTID();	// returns -1 on error
};

class SMBtdisPacket : public SMBPacket     /* tree disconnect */
{
public:
	SMBtdisPacket(uint16 tid=0xFFFF, uint8 dial=0);
};

class SMBnegprotPacket : public SMBPacket  /* negotiate protocol */
{
protected:
	static const int NbDialect;
	static const char *dialect[];
public:
	SMBnegprotPacket(uint8 dial=0);
	int16 getAcceptedDialect();
	int16 getSecurityMode();
	int16 getMaxMessageSize();
	uint8* getCryptKey(int &length);
	uint32 getSessionKey();
	uint8 isReadRawPossible();
};

class SMBdskattrPacket : public SMBPacket  /* get disk attributes */
{
};

class SMBsearchPacket : public SMBPacket   /* search directory */
{
};

class SMBsplopenPacket : public SMBPacket  /* open print spool file */
{
};

class SMBsplwrPacket : public SMBPacket    /* write to print spool file */
{
};

class SMBsplclosePacket : public SMBPacket /* close print spool file */
{
};

class SMBsplretqPacket : public SMBPacket  /* return print queue */
{
};

class SMBsendsPacket : public SMBPacket    /* send single block message */
{
};

class SMBsendbPacket : public SMBPacket    /* send broadcast message */
{
};

class SMBfwdnamePacket : public SMBPacket  /* forward user name */
{
};

class SMBcancelfPacket : public SMBPacket  /* cancel forward */
{
};

class SMBgetmacPacket : public SMBPacket   /* get machine name */
{
};

class SMBsendstrtPacket : public SMBPacket /* send start of multi-block message */
{
};

class SMBsendendPacket : public SMBPacket  /* send end of multi-block message */
{
};

class SMBsendtxtPacket : public SMBPacket  /* send text of multi-block message */
{
};

class SMBlockreadPacket : public SMBPacket       /* lock then read data */
{
};

class SMBwriteunlockPacket : public SMBPacket    /* write then unlock data */
{
};

class SMBreadBrawPacket : public SMBPacket       /* read block raw */
{
public:
	SMBreadBrawPacket(uint16 tid=0, uint16 fid=0, uint32 offset=0, uint16 count=0, uint8 dial=0);
};

class SMBreadBmpxPacket : public SMBPacket       /* read block multiplexed */
{
};

class SMBreadBsPacket : public SMBPacket         /* read block (secondary response) */
{
};

class SMBwriteBrawPacket : public SMBPacket      /* write block raw */
{
public:
	SMBwriteBrawPacket(uint16 tid=0, uint16 fid=0, uint32 offset=0, uint16 totalLength=0, uint8 dial=0);
};

class SMBwriteBmpxPacket : public SMBPacket      /* write block multiplexed */
{
};

class SMBwriteBsPacket : public SMBPacket        /* write block (secondary request) */
{
};

class SMBwriteCPacket : public SMBPacket         /* write complete response */
{
};

class SMBsetattrEPacket : public SMBPacket       /* set file attributes expanded */
{
};

class SMBgetattrEPacket : public SMBPacket       /* get file attributes expanded */
{
};

class SMBlockingXPacket : public SMBPacket       /* lock/unlock byte ranges and X */
{
};

class SMBtransPacket : public SMBPacket          /* transaction - name, bytes in/out */
{
public:
	// At this level, we don't need to know if there will be
	// secondary requests. Just make a packet is what is requested
	SMBtransPacket(uint16 tid, const char* transactName,
		uint16 *setup, uint16 setupLength, // # of uint16
		uint16 totalParameterCount,
		uint16 totalDataCount,
		uint8* param, uint16 paramLength,
		uint8* data, uint16 dataLength, uint8 dial=0);
};

class SMBtranssPacket : public SMBPacket         /* transaction (secondary request/response) */
{
public:
	// Just make a packet is what is requested here as well
	SMBtranssPacket(uint16 tid, uint16 totalParameterCount=0,
		uint16 totalDataCount=0,
		uint16 paramDisplacement=0, // start of these param / total param
		uint16 dataDisplacement=0,
		uint8* param=0, uint16 paramLength=0,
		uint8* data=0, uint16 dataLength=0, uint8 dial=0);
	SMBtranssPacket() {}
	uint16 getSetupCount();
	uint16* getSetup();
	uint16 getTotalParamCount();
	uint16 getTotalDataCount();
	uint16 getParamCount();
	uint16 getDataCount();
	uint16 getParamDisplacement();
	uint16 getDataDisplacement();
	uint8* getParam();
	uint8* getData();
};

class SMBioctlPacket : public SMBPacket          /* IOCTL */
{
};

class SMBioctlsPacket : public SMBPacket         /* IOCTL  (secondary request/response) */
{
};

class SMBcopyPacket : public SMBPacket           /* copy */
{
};

class SMBmovePacket : public SMBPacket           /* move */
{
};

class SMBechoPacket : public SMBPacket           /* echo */
{
};

class SMBwriteclosePacket : public SMBPacket     /* Write and Close */
{
};

class SMBopenXPacket : public SMBPacket          /* open and X */
{
};

class SMBreadXPacket : public SMBPacket          /* read and X */
{
};

class SMBwriteXPacket : public SMBPacket         /* write and X */
{
};

class SMBsesssetupPacket : public SMBPacket      /* Session Set Up & X (including User Logon) */
{
public:
	SMBsesssetupPacket(uint32 sessionKey=0, const char* account=0, uint8 *pass=0, uint16 passLen=0, uint16 UID=0, uint8 dial=0);
};

class SMBtconXPacket : public SMBtconPacket          /* tree connect and X */
{
protected:
	void update();	// ancestor virtual
public:
	SMBtconXPacket(const char* chemin=0, const uint8* pass=0, uint32 length=0, const char* dev="A:", uint8 dial=0);
	virtual uint16 getMaxMessageSize(); // returns 0 on error
	virtual uint16 getTID();	// returns -1 on error
};

class SMBffirstPacket : public SMBPacket         /* find first */
{
};

class SMBfuniquePacket : public SMBPacket        /* find unique */
{
};

class SMBfclosePacket : public SMBPacket         /* find close */
{
};

class SMBinvalidPacket : public SMBPacket        /* invalid command */
{
};

class SMBtrans2Packet : public SMBtransPacket    /* transaction2 - function, byte in/out */
{
	// SMBtrans will put the good value in the smb_com field
	// This class exists only because there are 2 different SMBs,
	// but the packet is the same...
	SMBtrans2Packet(uint16 tid,
		uint16 *setup, uint16 setupLength, // # of uint16
		uint16 totalParameterCount,
		uint16 totalDataCount,
		uint8* param, uint16 paramLength,
		uint8* data, uint16 dataLength, uint8 dial=0)
	: SMBtransPacket(tid, 0, setup, setupLength,
		totalParameterCount, totalDataCount,
		param, paramLength, data, dataLength, dial) {}
};

class SMBtranss2Packet : public SMBtranssPacket  /* transaction2 (secondary request/response*/
{
	// same as above
	SMBtranss2Packet(uint16 tid, uint16 totalParameterCount=0,
		uint16 totalDataCount=0,
		uint16 paramDisplacement=0, // start of these param / total param
		uint16 dataDisplacement=0,
		uint8* param=0, uint16 paramLength=0,
		uint8* data=0, uint16 dataLength=0, uint8 dial=0)
	: SMBtranssPacket(tid, totalParameterCount,
		totalDataCount, paramDisplacement, dataDisplacement,
		param, paramLength, data=0, dataLength, dial) {}
	SMBtranss2Packet() {}
};

class SMBfindclosePacket : public SMBPacket      /* find close */
{
};

class SMBfindnclosePacket : public SMBPacket     /* find notify close */
{
};

class SMBuloggoffXPacket : public SMBPacket      /* User logoff and X */
{
};

#endif
#endif // __SMBPACKET_H__
