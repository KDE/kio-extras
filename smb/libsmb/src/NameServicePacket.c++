/*
	This file is part of the smb library
    Copyright (C) 1999  Nicolas Brodu
    brodu@iie.cnam.fr

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

#include <string.h>
#include "order.h"
#include "NameServicePacket.h"

ResourceRecord::ResourceRecord(const char *n, uint16 t, uint16 c, uint16 l, uint8* d, uint16 dl)
{
	nameLength=strlen(n)+1;;
	rrName=new char[nameLength];
	memcpy(rrName,n,nameLength);
	rrType=t; rrClass=c; ttl=l;
	dataLength=dl;
	data=new uint8[dl];
	memcpy(data,d,dl);
}

ResourceRecord::~ResourceRecord()
{
	if (rrName) delete rrName;
	if (data) delete data;
}

uint8* ResourceRecord::build()
{
	uint8* ret=new uint8[nameLength+10+dataLength];
	memcpy(ret,rrName,nameLength);
	// warning from gcc with (uint16*) on left side
	uint16 *p16 = (uint16 *)((uint8*)ret+nameLength);
	*(p16++)=BIGEND16(rrType);
	*p16=BIGEND16(rrClass);	
	uint32 *p32 = (uint32 *)((uint8*)ret+nameLength+4);
	*p32=BIGEND32(ttl);;
	*(p16+3) = BIGEND16(dataLength);
	memcpy(ret+nameLength+10,data,dataLength);
	return ret;
}

uint16 ResourceRecord::getLength()
{
	return nameLength+dataLength+10;
}


NameServicePacket::NameServicePacket()
{
	name_trn_id=0; opcode=0; nmFlags=0; rcode=0;
	qdcount=0; ancount=0; nscount=0; arcount=0;
	questionName=0; questionType=0; questionClass=0;
	questionNameLength=0;
	answerRR=0; authorityRR=0; additionalRR=0;
}


// Get the real length of the packet in byte
uint16 NameServicePacket::getLength()
{
	uint16 ret=12;
	if (questionName) ret+=6+questionNameLength;
	if (answerRR) ret+=answerRR->getLength();
	if (authorityRR) ret+=authorityRR->getLength();
	if (additionalRR) ret+=additionalRR->getLength();
	return ret;
}

uint8* NameServicePacket::packet()
{
	if (modified) update(); // in case subclasses would use it
	uint8 *ret = new uint8[getLength()];
	// Build Header first
	ret[0]=(uint8)(name_trn_id>>8);   // high byte
	ret[1]=(uint8)(name_trn_id&0xFF); // low byte
	ret[2]=((uint32)(opcode&0x1F)<<3) | ((uint32)((nmFlags&0x7F)>>4));
	ret[3]=((uint32)(nmFlags&0x7F)<<4) | ((uint32)(rcode&0x0F));
	ret[4]=(uint8)(qdcount>>8);   // high byte
	ret[5]=(uint8)(qdcount&0xFF); // low byte
	ret[6]=(uint8)(ancount>>8);   // high byte
	ret[7]=(uint8)(ancount&0xFF); // low byte
	ret[8]=(uint8)(nscount>>8);   // high byte
	ret[9]=(uint8)(nscount&0xFF); // low byte
	ret[10]=(uint8)(arcount>>8);   // high byte
	ret[11]=(uint8)(arcount&0xFF); // low byte
	// Complete packet
	if (questionName)
	{
		memcpy(ret+12, questionName, questionNameLength);
		ret[12+questionNameLength]=(uint8)(questionType>>8);   // high byte
		ret[13+questionNameLength]=(uint8)(questionType&0xFF); // low byte
		ret[14+questionNameLength]=(uint8)(questionClass>>8);   // high byte
		ret[15+questionNameLength]=(uint8)(questionClass&0xFF); // low byte
	}
	uint8 *rr;
	uint8* pos=ret+12+questionNameLength;
	if (answerRR)
	{
		rr=answerRR->build();
		uint16 len=answerRR->getLength();
		memcpy(pos,rr,len);
		pos+=len;
		delete rr;
	}
	if (authorityRR)
	{
		rr=authorityRR->build();
		uint16 len=authorityRR->getLength();
		memcpy(pos,rr,len);
		pos+=len;
		delete rr;
	}
	if (additionalRR)
	{
		rr=additionalRR->build();
		uint16 len=additionalRR->getLength();
		memcpy(pos,rr,len);
		delete rr;
	}
	return ret;
}



/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         NAME_TRN_ID           |0|  0x0  |0|0|1|0|0 0|B|  0x0  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0001               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0000               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   /                         QUESTION_NAME                         /
   /                                                               /
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           NB (0x0020)         |        IN (0x0001)            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

NameQueryPacket::NameQueryPacket(const char* name, uint8 broadcast, uint16 id, bool groupFlag)
{
	name_trn_id=id;
	nmFlags=0x10+(broadcast&1); qdcount=1;
	questionType=0x20; questionClass=1;
//	isGroup=groupFlag;
	questionName=NBName(name, groupFlag);
	questionNameLength=strlen(questionName)+1;
}

char *NameQueryPacket::getQueryNBName()
{
	if (!questionName) return 0;
	char *ret=new char[strlen(questionName)+1];
	strcpy(ret,questionName);
	return ret;
}
/*
void NameQueryPacket::setQueryNBName(const char *n, bool groupFlag)
{
	if (questionName) delete questionName;
	isGroup=groupFlag;
	questionName=new char[strlen(n)+1];
	questionNameLength=strlen(n)+1;
	strcpy(questionName,n);
}
*/
NameConflictDemand::NameConflictDemand(const char* name, uint16 id)
{
	name_trn_id=id; opcode=0x15; nmFlags=0x58; rcode=0x7;
	arcount=1;
	char *n=NBName(name);
	uint8* data=new uint8[6];
	for (int i=0; i<6; i++) data[i]=0;
	answerRR=new ResourceRecord(n,0x20,1,0, data,6);
	delete data;
	delete n;
}

