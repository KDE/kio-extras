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

#include <unistd.h> // for getpid(), getuid(), getgid()
#include <fcntl.h>	// need open() flags
#include <string.h>
#include <time.h>	// needed for mktime() and tm, time_t structs
#include "SMBPacket.h"
#include "SMBCodes.h"

#ifdef DEBUG
#include <iostream.h>
#include <stdio.h>
#endif

SMBPacket::SMBPacket(uint8 dial)
{
	dialect=dial;

	smb_pid=getpid();
	smb_mid=0;  // We'll see if we need multiplexed connections later
	smb_uid=0;	// We are not registered yet
	smb_tid=0;	// We are not registered yet
	smb_err=SUCCESS;
	smb_rcls=SUCCESS;
	smb_com=SMBnegprot; // Should be overrided later on
	
	smb_wct=0;
	smb_bcc=0;
	smb_buf=0;
	smb_vwv=0;
	
	modified=1;
}

SMBPacket::~SMBPacket()
{
	if (smb_buf) delete smb_buf;
	if (smb_vwv) delete smb_vwv;
}


void SMBPacket::update()
{
	if (!modified) return;
	if (data) delete data;
	// The 3 at the end of next line stands for size of smb_wct and smb_bcc
	length=32+((uint16)smb_wct)*2+smb_bcc+3;
	data = new uint8[length];
	data[0]=0xFF; data[1]='S'; data[2]='M'; data[3]='B';
	data[4]=smb_com;
	data[5]=smb_rcls;
	data[6]=0;	// reserved	- flags in later protocols
	data[7]=(uint8)(smb_err&0xFF); // little endian
	data[8]=(uint8)(smb_err>>8); // little endian
	data[9]=0; // reserved
	for (int i=10; i<24; i++) data[i]=0; // reserved
	data[24]=(uint8)(smb_tid&0xFF); // little endian
	data[25]=(uint8)(smb_tid>>8); // little endian
	data[26]=(uint8)(smb_pid&0xFF); // little endian
	data[27]=(uint8)(smb_pid>>8); // little endian
	data[28]=(uint8)(smb_uid&0xFF); // little endian
	data[29]=(uint8)(smb_uid>>8); // little endian
	data[30]=(uint8)(smb_mid&0xFF); // little endian
	data[31]=(uint8)(smb_mid>>8); // little endian
	data[32]=smb_wct;
	if (smb_wct) memcpy(data+33,(uint8*)smb_vwv,((uint16)smb_wct)*2);
	data[33+((uint16)smb_wct)*2]=(uint8)(smb_bcc&0xFF);
	data[34+((uint16)smb_wct)*2]=(uint8)(smb_bcc>>8);
	if (smb_bcc) memcpy(data+35+((uint16)smb_wct)*2,smb_buf,smb_bcc);
	modified=0;
}

// converts parents data field into our structure
int SMBPacket::parse(SessionPacket *p)
{
	if (SessionPacket::parse(p)==-1) return -1;
	
	// First check for a consistent length
	if (length<35) 	// min size
	{
		errno=ERRCMD;	// not in smb format
		return -1;
	}
	smb_wct=data[32];
	if (smb_wct)
	{
		if (length<35+((uint16)smb_wct)*2)
		{
			errno=ERRCMD;	// not in smb format
			return -1;
		}
		if (smb_vwv) delete smb_vwv;
		smb_vwv=new uint16[((uint16)smb_wct)*2];
#ifdef WORDS_BIGENDIAN
		for (int i=0; i<(uint16)smb_wct; i++)
			smb_vwv[i]=(uint16)(*(data+33+i*2)) | (uint16)((*(data+34+i*2))<<8);
#else
		memcpy(smb_vwv,data+33,((uint16)smb_wct)*2);
#endif
	}	
	// little endian
	smb_bcc=(uint16)(data[33+((uint16)smb_wct)*2]) | ((uint16)(data[34+((uint16)smb_wct)*2])<<8);
	if (smb_bcc)
	{
		if (length<35+((uint16)smb_wct)*2+smb_bcc)
		{
			errno=ERRCMD;	// not in smb format
			return -1;
		}
		if (smb_buf) delete smb_buf;
		smb_buf=new uint8[smb_bcc];
		memcpy(smb_buf,data+35+((uint16)smb_wct)*2,smb_bcc);
	}
#if DEBUG >= 5
	cout<<"Parse : word count : "<<(uint16)smb_wct<<" byte count : "<<(uint16)smb_bcc<<"\n";;
#endif
	
	// now update remaining fields
	smb_com=data[4];
	smb_rcls=data[5];
	smb_err=(uint16)(data[7]) | ((uint16)(data[8])<<8);
	smb_tid=(uint16)(data[24]) | ((uint16)(data[25])<<8);
	smb_pid=(uint16)(data[26]) | ((uint16)(data[27])<<8);
	smb_uid=(uint16)(data[28]) | ((uint16)(data[29])<<8);
	smb_mid=(uint16)(data[30]) | ((uint16)(data[31])<<8);

	modified=0;	
	return 1;
}


uint8 SMBPacket::getSMBType()
{
	if (modified) update();
	return smb_com;
}

uint16 SMBPacket::getError()
{
	if (modified) update();
	return smb_err;
}


// Parent virtuals
uint16 SMBPacket::getDataLength()
{
	if (modified) update();
	return length;
}

uint8 *SMBPacket::getData()
{
	if (modified) update();
	return data;
}




/**********************
Now the command Packets
***********************/

/* open file */
SMBopenPacket::SMBopenPacket(uint16 TID, const char *filename, int flags, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBopen;
	smb_tid=TID;
	
	smb_wct=2;
	smb_vwv=new uint16[2];
	uint16 access=0;
	if (flags & O_RDONLY) access=0;
	if (flags & O_WRONLY) access=0x5001;
	if (flags & O_RDWR) access=0x5002;
	smb_vwv[0]=access; // will be converted in little endian later
	smb_vwv[1]=0x27;   // DOS attributes for all files types except 'directory' & 'volume'
	
	if (filename)
	{
		smb_bcc=2+strlen(filename);
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		strcpy((char*)(smb_buf+1), filename);
		smb_buf[smb_bcc-1]=0;
	}
	else
	{
		smb_bcc=2;
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		smb_buf[1]=0;	// empty string
	}
	modified=1;
}

uint16 SMBopenPacket::getFID()
{
	if (smb_wct>=7)
	{
		errno=SUCCESS;
		return smb_vwv[0];
	}
	else
	{
		errno=ERRbadformat;
		return 0xFFFF;
	}
}


/* create file */
SMBcreatePacket::SMBcreatePacket(uint16 TID, const char *filename, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBcreate;
	smb_tid=TID;
	
	smb_wct=3;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=0x20;   // DOS attribute for "archive"
	smb_vwv[1]=0;   // creation time, optional
	smb_vwv[2]=0;   // creation date, optional
	
	if (filename)
	{
		smb_bcc=2+strlen(filename);
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		strcpy((char*)(smb_buf+1), filename);
		smb_buf[smb_bcc-1]=0;
	}
	else
	{
		smb_bcc=2;
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		smb_buf[1]=0;	// empty string
	}
	modified=1;
}

uint16 SMBcreatePacket::getFID()
{
	if (smb_wct>=1)
	{
		errno=SUCCESS;
		return smb_vwv[0];
	}
	else
	{
		errno=ERRbadformat;
		return 0xFFFF;
	}
}


SMBgetatrPacket::SMBgetatrPacket(uint16 tid, const char *filename, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBgetatr;
	smb_tid=tid;
	
	smb_wct=0;
	
	if (filename)
	{
		smb_bcc=2+strlen(filename);
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		strcpy((char*)(smb_buf+1), filename);
		smb_buf[smb_bcc-1]=0;
	}
	else
	{
		smb_bcc=2;
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		smb_buf[1]=0;	// empty string
	}
	modified=1;
}

SMBwritePacket::SMBwritePacket(uint16 tid, uint16 fid, uint32 offset, uint8* buf, uint16 length, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBwrite;
	smb_tid=tid;
	smb_wct=5;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=fid;
	smb_vwv[1]=length;
	smb_vwv[2]=(uint16)(offset&0xFFFF);
	smb_vwv[3]=(uint16)(offset>>16);
	smb_vwv[4]=0;
	smb_bcc=length+3; // plus data description
	smb_buf=new uint8[smb_bcc];
	smb_buf[0]=1;	// data is a block
	smb_buf[1]=(uint8)(length&0xFF);    // length in little endian
	smb_buf[2]=(uint8)(length>>8)&0xFF;
	memcpy(smb_buf+3,buf,length);
	modified=1;
}

int32 SMBwritePacket::getWrittenCount()
{
	if (smb_wct>=1)
	{
		errno=SUCCESS;
		return smb_vwv[0];
	}
	else
	{
		errno=ERRbadformat;
		return -1;
	}
}

SMBreadPacket::SMBreadPacket(uint16 tid, uint16 fid, uint32 offset, uint16 count, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBread;
	smb_tid=tid;
	smb_wct=5;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=fid;
	smb_vwv[1]=count;
	smb_vwv[2]=(uint16)(offset&0xFFFF);
	smb_vwv[3]=(uint16)(offset>>16);
	smb_vwv[4]=0;
	modified=1;
}

uint16 SMBreadPacket::getReadCount()
{
	if (smb_bcc>=3)
	{
		errno=SUCCESS;
		// should we return the count in smb_vwv[0] or
		// the actual data length ?
		// I'd trust the server to fill in data length, so...
//		cout<<"count : "<<(uint16)smb_vwv[0]<<"\n";
		uint16 len=(((uint16)(smb_buf[1])&0xFF)|((uint16)(smb_buf[2])<<8));
//		cout<<"len : "<<(uint16)len<<"\n";
		return len;
	}
	else
	{
		errno=ERRbadformat;
		return 0;
	}
}

uint8 *SMBreadPacket::getReadData()
{
	if (smb_bcc>=3)
	{
		uint16 n=getReadCount();
		if (smb_bcc!=3+n)
		{
			errno=ERRbadformat;
			return 0;
		}
		uint8 *ret=new uint8[n];
		memcpy(ret, smb_buf+3, n);
		errno=SUCCESS;
		return ret;
	}
	else
	{
		errno=ERRbadformat;
		return 0;
	}
}


SMBreadBrawPacket::SMBreadBrawPacket(uint16 tid, uint16 fid, uint32 offset, uint16 count, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBreadBraw;
	smb_tid=tid;
	smb_wct=8;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=fid;
	smb_vwv[1]=(uint16)(offset&0xFFFF);
	smb_vwv[2]=(uint16)(offset>>16);
	smb_vwv[3]=count;
	smb_vwv[4]=0;
	smb_vwv[5]=0;
	smb_vwv[6]=0;
	smb_vwv[7]=0;
	modified=1;
}

SMBwriteBrawPacket::SMBwriteBrawPacket(uint16 tid, uint16 fid, uint32 offset, uint16 totalLength, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBwriteBraw;
	smb_tid=tid;
	smb_wct=12;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=fid;
	smb_vwv[1]=totalLength; // total data that will be sent
#if DEBUG >= 6
	cout<<"SMBwriteBrawPacket : totalLength="<<totalLength<<"\n";
#endif
	smb_vwv[2]=0; // reserved
	smb_vwv[3]=(uint16)(offset&0xFFFF);
	smb_vwv[4]=(uint16)(offset>>16);
	smb_vwv[5]=0xFFFF; // timeout in little endian
	smb_vwv[6]=0x0FFF; // timeout - end
	smb_vwv[7]=1; // write mode = send final confirmation (never too cautious with smb...)
	smb_vwv[8]=0; // reserved
	smb_vwv[9]=0; // reserved
	smb_vwv[10]=0; // 0 bytes send in this request, we won't "optimize" for now...
	smb_vwv[11]=0; // offset to data from header start : not important !
	modified=1;
}


struct stat *SMBgetatrPacket::getStat()
{
	if (smb_wct>=10)
	{
		errno=SUCCESS;
		struct stat *s=(struct stat*)(new uint8[sizeof(struct stat)]);
		s->st_mode=0644; //default
		// archive attribute mapped to 0644
		if (smb_vwv[0]&0x20) s->st_mode|=0644;
		// read-only attribute removes write permission
		if (smb_vwv[0]&0x01) s->st_mode&=077666;
		// directory attribute mapped to 040755
		if (smb_vwv[0]&0x10) s->st_mode|=040755;
		// Anyone has any idea about what volume, hidden and
		// system should be mapped to ???
		struct tm footm;
		footm.tm_sec=(smb_vwv[1]&0x1F)<<1; // 2 sec increments
		footm.tm_min=(smb_vwv[1]&0x7E0)>>5; // minutes
		footm.tm_hour=(smb_vwv[1]&0xF800)>>11; // hour
		footm.tm_mday=smb_vwv[2]&0x1F; // day of month
		footm.tm_mon=((smb_vwv[1]&0x1E0)>>5)-1; // month 1..12 => 0..11
		// And make us ready for year 2100 bug...
		footm.tm_year=((smb_vwv[1]&0xFE00)>>9)+80; // weird 0..119 interval from 1980
		footm.tm_isdst=-1; // daylight info not available
		s->st_atime=mktime(&footm); // computes wday and yday and converts to time_t type
		s->st_mtime=s->st_atime; // same date/time for last access
		s->st_ctime=s->st_atime; // modification and change
		s->st_uid=getuid(); // so we're sure user exist, access rights have been set
		s->st_gid=getgid(); // It would be better if owner was "nobody"
		s->st_size=(uint32)(smb_vwv[3]) | ((uint32)(smb_vwv[4]) << 16);
		s->st_blksize=1024;            // why not ?
		s->st_blocks=s->st_size >> 10;
		s->st_rdev=0100000; // regular file
		return s;
	}
	else
	{
		errno=ERRbadformat;
		return 0;
	}
}


SMBclosePacket::SMBclosePacket(uint16 fid)
{
	smb_com=SMBclose;
	smb_wct=3;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=fid;
	smb_vwv[1]=0; // time and date will
	smb_vwv[2]=0; // be set by server
	modified=1;
}

SMBunlinkPacket::SMBunlinkPacket(uint16 TID, const char *filename, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBunlink;
	smb_tid=TID;
	
	smb_wct=1;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=0x26; // DOS attribute for "archive", "hidden", "system"
	
	if (filename)
	{
		smb_bcc=2+strlen(filename);
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		strcpy((char*)(smb_buf+1), filename);
		smb_buf[smb_bcc-1]=0;
	}
	else
	{
		smb_bcc=2;
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		smb_buf[1]=0;	// empty string
	}
	modified=1;
}


SMBmkdirPacket::SMBmkdirPacket(uint16 TID, const char *pathname, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBmkdir;
	smb_tid=TID;
	
	smb_wct=0;
	
	if (pathname)
	{
		smb_bcc=2+strlen(pathname);
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		strcpy((char*)(smb_buf+1), pathname);
		smb_buf[smb_bcc-1]=0;
	}
	else
	{
		smb_bcc=2;
		smb_buf=new uint8[smb_bcc];
		smb_buf[0]=4;	// data is ASCIIZ
		smb_buf[1]=0;	// empty string
	}
	modified=1;
}


SMBrmdirPacket::SMBrmdirPacket(uint16 TID, const char *pathname, uint8 dial) : SMBmkdirPacket(TID, pathname, dial)
{
	smb_com=SMBrmdir;
	modified=1;
}

SMBmvPacket::SMBmvPacket(uint16 TID, const char *srcfile, const char *dstfile, uint8 dial)  : SMBPacket(dial)
{
	smb_com=SMBmv;
	smb_tid=TID;
	
	if ((!srcfile) || (!dstfile)) return; // no need to go further on
	
	smb_wct=1;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=6; // search attributes : include "system" and "hidden" files
	
	smb_bcc=4+strlen(srcfile)+strlen(dstfile);
	smb_buf=new uint8[smb_bcc];
	smb_buf[0]=4;	// data is ASCIIZ
	strcpy((char*)(smb_buf+1), srcfile);
	smb_buf[2+strlen(srcfile)]=4;	// data is ASCIIZ
	strcpy((char*)(smb_buf+3+strlen(srcfile)), dstfile);
	smb_buf[smb_bcc-1]=0;
	modified=1;
}

const int SMBnegprotPacket::NbDialect=6;
char *SMBnegprotPacket::dialect[] =
{
"PC NETWORK PROGRAM 1.0",
"PCLAN1.0",
"MICROSOFT NETWORKS 1.03",
"MICROSOFT NETWORKS 3.0",
"DOS LANMAN1.0",
"LANMAN1.0"
/*"LM1.2X002",
"DOS LM1.2X002",
"DOS LANMAN2.1",
"LANMAN2.1",
"Samba",
"NT LM 0.12",
"NT LANMAN 1.0"*/
};
SMBnegprotPacket::SMBnegprotPacket(uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBnegprot;
	smb_bcc=0;
	for (int i=0; i<NbDialect; i++) smb_bcc+=strlen(dialect[i])+2;
	smb_buf = new uint8[smb_bcc];
	char *tmp=(char*)smb_buf;
	for (int i=0; i<NbDialect; i++)
	{
		*tmp=2;	// Data type = dialect
		strcpy(tmp+1, dialect[i]);
		tmp+=strlen(dialect[i])+2;
	}
	modified=1;
}

int16 SMBnegprotPacket::getAcceptedDialect()
{
	if (smb_wct>=1)
	{
#if DEBUG >= 4
	cout<<"Negotiation Protocol :\n"<<"wct : "<<(int)smb_wct<<"\n";
	if (smb_wct>=13)
	{
		cout<<"index : "<<smb_vwv[0]<<"\n";
		cout<<"Security  : "<<smb_vwv[1]<<"\n";
		cout<<"MaxBufSize : "<<smb_vwv[2]<<"\n";
		cout<<"MaxMux : "<<smb_vwv[3]<<"\n";
		cout<<"MaxConnect : "<<smb_vwv[4]<<"\n";
		cout<<"RawMode : "<<smb_vwv[5]<<"\n";
		cout<<"SessionKey : "<<(uint32)((uint32)(smb_vwv[6]) | (uint32)(smb_vwv[7])<<16) <<"\n";
		cout<<"Time : "<<smb_vwv[8]<<smb_vwv[9]<<"\n";
		cout<<"Date : "<<smb_vwv[10]<<smb_vwv[11]<<"\n";
		cout<<"TimeZone : "<<smb_vwv[12]<<"\n";
	}
	cout<<"smb_buf length : "<<(int)smb_bcc<<"\n";
	cout<<"smb_buf : ";
	for (int i=0; i<smb_bcc; i++) printf("%X ",smb_buf[i]); cout<<"\n";
#endif
		errno=SUCCESS;
		return smb_vwv[0];
	}
	else
	{
		errno=ERRbadformat;
		return -1;
	}
}

int16 SMBnegprotPacket::getSecurityMode()
{
	if (smb_wct>=2)
	{
		errno=SUCCESS;
		return smb_vwv[1];
	}
	else
	{
		errno=ERRbadformat;
		return -1;
	}
}

int16 SMBnegprotPacket::getMaxMessageSize()
{
	if (smb_wct>=3)
	{
		errno=SUCCESS;
		return smb_vwv[2];
	}
	else
	{
		errno=ERRbadformat;
		return -1;
	}
}

// return the key if any, null otherwise
// the length of the key will be return throught
// the int in argument
uint8* SMBnegprotPacket::getCryptKey(int &length)
{
	uint8* ret=0;
	length=0;
	if (smb_bcc>=8)
	{
		ret=new uint8[8];
		memcpy((char*)ret,(char*)smb_buf,8);
		length=8;
	}
	return ret;
}

uint32 SMBnegprotPacket::getSessionKey()
{
	if (smb_wct>=7)
		return (uint32)((uint32)(smb_vwv[6]) | (uint32)(smb_vwv[7])<<16);
	else
	{
		errno=ERRbadformat;
		return 0xFFFFFFFF;
	}
}

uint8 SMBnegprotPacket::isReadRawPossible()
{
	if (smb_wct>=6)
		return (uint8)(smb_vwv[5] & 1);
	else
	{
		errno=ERRbadformat;
		return 0; // 0 and not 0xFF, so that the program has a chance to work
	}             // since it will not use readRaw (boolean).
}


SMBtconPacket::SMBtconPacket(const char* chemin, const uint8* pass, uint32 length, const char* dev, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBtcon;
	path=0; password=0; device=0;
	passLen=0;
	setPath(chemin);
	setPassword(pass, length);
	setDevice(dev);
}

void SMBtconPacket::setPath(const char *chemin)
{
	if (path) delete path;
	if (chemin)
	{
		path = new char[strlen(chemin)+1];
		strcpy(path,chemin);
	}
	else
	{
		path = new char[1];
		path[0]=0;
	}
	modified=1;
}

void SMBtconPacket::setPassword(const uint8 *pass, uint32 length)
{
	if (password) delete password;
	if (pass)
	{
		passLen=length;
		password = new uint8[passLen];
		memcpy(password,pass,passLen);
	}
	else
	{
		passLen=1;
		password = new uint8[1];
		password[0]=0;
	}
	modified=1;
}

void SMBtconPacket::setDevice(const char *dev)
{
	if (device) delete device;
	if (dev)
	{
		device = new char[strlen(dev)+1];
		strcpy(device,dev);
	}
	else
	{
		device = new char[1];
		device[0]=0;
	}
	modified=1;
}

void SMBtconPacket::update()
{
	if (!modified) return;
	if (smb_buf) delete (smb_buf);
	if (smb_vwv) delete (smb_vwv);
	if (!path) setPath("");
	if (!password) setPassword(0,0);
	if (!device) setDevice("");
	smb_wct=0;
	int l1=strlen(path)+1;
	int l2=passLen;
	int l3=strlen(device)+1;
	smb_bcc=l1+l2+l3+3;	// +3 for the 3 data id
	smb_buf=new uint8[smb_bcc];
	smb_buf[0]=4;	// data id for ASCIIZ
	smb_buf[l1+1]=4;	// data id for ASCIIZ
	smb_buf[l1+l2+2]=4;	// data id for ASCIIZ
	memcpy(smb_buf+1,path,l1);
	memcpy(smb_buf+l1+2,password,l2);
	memcpy(smb_buf+l1+l2+3,device,l3);
	SMBPacket::update();
}

uint16 SMBtconPacket::getMaxMessageSize()
{
	if (smb_wct) return smb_vwv[0];
	else
	{
		errno=ERRbadformat;
		return 0;	// can't put -1, 0xFFFF is a valid answer
	}
}

uint16 SMBtconPacket::getTID()
{
	if (smb_wct>=2) return smb_vwv[1];
	else
	{
		errno=ERRbadformat;
		return (uint16)-1;
	}
}

SMBtconXPacket::SMBtconXPacket(const char* chemin, const uint8* pass, uint32 length, const char* dev, uint8 dial) :
		SMBtconPacket(chemin, pass, length, dev, dial)
{
	smb_com=SMBtconX;
#if DEBUG>=6
	printf("SMBtconXPacket : constructor :\n");
	cout<<"passLen : "<<passLen<<"\npass :\n";
	for (uint16 i=0; i<passLen; i++) printf("%X ",password[i]);
	printf("\n");
#endif
}

void SMBtconXPacket::update()
{
	if (!modified) return;
	if (smb_buf) delete (smb_buf);
	if (smb_vwv) delete (smb_vwv);
	if (!path) setPath("");
	if (!password) setPassword(0,0);
	if (!device) setDevice("");
	smb_wct=4;
	smb_vwv=new uint16[4];
	smb_vwv[0]=0x00FF;	// AndX command, will be stored in litle endian
	smb_vwv[1]=0;	// offset to next command wordcount
	smb_vwv[2]=0;	// flags : 0
	smb_vwv[3]=(uint16)passLen;	// that's why TreeconX and not Treecon
	int l1=passLen;
	int l2=strlen(path)+1;
	int l3=strlen(device)+1;
	smb_bcc=l1+l2+l3;	// only lengths, no data id here !!!
	smb_buf=new uint8[smb_bcc];
	memcpy(smb_buf,password,passLen);
#if DEBUG>=6
	printf("SMBtconXPacket : Pass sent to server :\n");
	for (uint16 i=0; i<passLen; i++) printf("%X ",password[i]);
	printf("\n");
#endif
	memcpy(smb_buf+passLen,path,l2);
	memcpy(smb_buf+passLen+l2,device,l3);
	SMBPacket::update();
}

uint16 SMBtconXPacket::getMaxMessageSize()
{
	errno=ERRbadformat; // only for tcon... not present in tconX !
	return 0;	// can't put -1, 0xFFFF is a valid answer
}

uint16 SMBtconXPacket::getTID()
{
	return smb_tid; // no field in doc, ???
}

SMBtdisPacket::SMBtdisPacket(uint16 tid, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBtdis;
	smb_tid=tid;
	modified=1;
}


SMBsesssetupPacket::SMBsesssetupPacket(uint32 sessionKey, const char* account, uint8 *pass, uint16 passLen, uint16 UID, uint8 dial) : SMBPacket(dial)
{
	smb_com=SMBsesssetup;
	smb_uid=UID;
	
	smb_wct=10;
	smb_vwv=new uint16[10];
	smb_vwv[0]=0x00FF;	// AndX command, will be stored in litle endian
	smb_vwv[1]=0;	// offset to next command wordcount
	smb_vwv[2]=0xFFFF;	// max buf size, we don't care
	smb_vwv[3]=0;	// Max multiplexed pending request
	smb_vwv[4]=0;	// "Virtual Circuit"=tcp/ip connection number
	smb_vwv[5]=sessionKey&0xFFFF;	// Session key if V.C. above<>0
	smb_vwv[6]=(uint32)sessionKey>>16;
	smb_vwv[7]=passLen;	// Password length
	smb_vwv[8]=0; smb_vwv[9]=0;	// CIFS : reserved, Lanman1 : encrypt

/*	char *emptyString="";
	if (!account) account=emptyString;
	uint16 accLen=strlen(account)+1; //(account) ? strlen(account)+1 : 0;
	if (passLen==0) {
		pass=(uint8*)emptyString;
		passLen=1;
	}*/
	uint16 accLen=(account)?strlen(account)+1:0;
	if (!pass) passLen=0;
	smb_bcc=accLen+passLen;
	if (smb_bcc) smb_buf=new uint8[smb_bcc];
	if (passLen) memcpy(smb_buf,pass,passLen);
	if (accLen) strcpy((char*)smb_buf+passLen,account);

	modified=1;
#if DEBUG >=6
	print();
#endif
}


SMBtransPacket::SMBtransPacket(uint16 tid, const char* transactName,
		uint16 *setup, uint16 setupLength, // # of uint16
		uint16 totalParameterCount,
		uint16 totalDataCount,
		uint8* param, uint16 paramLength,
		uint8* data, uint16 dataLength, uint8 dial)
{
	smb_tid=tid;
	smb_com=(transactName)?SMBtrans:SMBtrans2;
	smb_wct=14+setupLength;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=totalParameterCount;
	smb_vwv[1]=totalDataCount;
	smb_vwv[2]=0x400; // max paramCount to return : limit it to
	smb_vwv[3]=0x400; // max dataCount : 1kB bytes because windows
	smb_vwv[4]=0X7F; // max setupCount : fills the answer with 0 !
	smb_vwv[5]=0;
	smb_vwv[6]=0xFFF; // timeout
	smb_vwv[7]=0xFFF; // timeout
	smb_vwv[8]=0; //reserved
	smb_vwv[9]=paramLength;
//	smb_vwv[10]=; // see below
	smb_vwv[11]=dataLength;
//	smb_vwv[12]=; // see below
	smb_vwv[13]=setupLength;
	for (int i=0; i<setupLength; i++) smb_vwv[14+i]=setup[i];
	int nameSpace=(transactName)?strlen(transactName)+1:0;
	nameSpace+=nameSpace&0x1; // pad to next uint16
	int paramSpace=paramLength+(paramLength&0x1);
	smb_bcc=nameSpace+paramSpace+dataLength;
	smb_buf=new uint8[smb_bcc];
	if (transactName) strcpy((char*)smb_buf,(char*)transactName);
	if (paramLength) memcpy(smb_buf+nameSpace,param,paramLength);
	if (dataLength) memcpy(smb_buf+nameSpace+paramSpace,data,dataLength);
	// offset from header start to parameters
	smb_vwv[10]=35+((uint16)smb_wct)*2+nameSpace;
	// offset from header start to data
	smb_vwv[12]=smb_vwv[10]+paramSpace;
	modified=1;
}


SMBtranssPacket::SMBtranssPacket(uint16 tid, uint16 totalParameterCount,
		uint16 totalDataCount,
		uint16 paramDisplacement, // start of these param / total param
		uint16 dataDisplacement,
		uint8* param, uint16 paramLength,
		uint8* data, uint16 dataLength, uint8 dial)
{
	smb_com=SMBtranss;
	smb_tid=tid;
	smb_wct=8;
	smb_vwv=new uint16[smb_wct];
	smb_vwv[0]=totalParameterCount;
	smb_vwv[1]=totalDataCount;
	smb_vwv[2]=paramLength;
//	smb_vwv[3]=; // see below
	smb_vwv[4]=paramDisplacement;
	smb_vwv[5]=dataLength;
//	smb_vwv[6]=; // see below
	smb_vwv[7]=dataDisplacement;
	int paramSpace=paramLength+(paramLength&0x1);
	smb_bcc=1+paramSpace+dataLength;
	smb_buf=new uint8[smb_bcc];
	smb_buf[0]=0;	// pad to uint16
	if (paramLength) memcpy(smb_buf+1,param,paramLength);
	if (dataLength) memcpy(smb_buf+1+paramSpace,data,dataLength);
	// offset from header start to parameters
	smb_vwv[10]=35+8*2+1;
	// offset from header start to data
	smb_vwv[12]=smb_vwv[10]+paramSpace;
	modified=1;
}

uint16 SMBtranssPacket::getSetupCount()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	if (smb_wct<10+smb_vwv[9]) {errno=ERRerror; return 0;}
	return smb_vwv[9];
}

uint16* SMBtranssPacket::getSetup()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	if (smb_wct<10+smb_vwv[9]) {errno=ERRerror; return 0;}
	return (uint16*)((uint16*)smb_vwv+10);
}

uint16 SMBtranssPacket::getTotalParamCount()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[0];
}

uint16 SMBtranssPacket::getTotalDataCount()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[1];
}

uint16 SMBtranssPacket::getParamCount()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[3];
}

uint16 SMBtranssPacket::getDataCount()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[6];
}

uint16 SMBtranssPacket::getParamDisplacement()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[5];
}

uint16 SMBtranssPacket::getDataDisplacement()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	return smb_vwv[8];
}

uint8* SMBtranssPacket::getParam()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	uint8* ret=new uint8[smb_vwv[3]];
	memcpy(ret,smb_buf+smb_vwv[4]-35-(uint16)(smb_wct)*2,smb_vwv[3]);
	return ret;
}

uint8* SMBtranssPacket::getData()
{
	if (smb_wct<10) {errno=ERRerror; return 0;}
	uint8* ret=new uint8[smb_vwv[6]];
	memcpy(ret,smb_buf+smb_vwv[7]-35-(uint16)(smb_wct)*2,smb_vwv[6]);
	return ret;
}
#endif
