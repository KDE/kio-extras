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

#include <string.h> // for memcpy
#include "SessionPacket.h"

#ifdef DEBUG
#include <stdio.h>
#include <iostream.h>
#endif

SessionPacket::SessionPacket(int8 t, uint8* d, int32 l)
{
	data=0;
	setType(t);
	setData(d,l);
	modified=1;
}

void SessionPacket::setType(int8 t)
{
	type=t;
	modified=1;
}

void SessionPacket::setData(uint8* d, int32 l)
{
	if (data) delete data;
	if ((l) && (d))
	{
		data = new uint8[l];
		memcpy(data, d, l);	// caller can dealloc any time
	}
	else data=0;
	length=l;
	modified=1;
}

SessionPacket::~SessionPacket()
{
	if (data) delete data;
}

uint8* SessionPacket::packet()
{
	if (modified) update();
	uint8 *ret = new uint8[length+4];
	// Build Header first
	ret[0]=type;
	// flags contain high-order bit
	ret[1]=(uint8)((length&0x10000)>>16);
	// now put length in big endian mode
	ret[2]=(uint8)((uint16)(length&0xFF00)>>8);
	ret[3]=(uint8)(length&0xFF);
	// Complete packet
	if (length) memcpy(ret+4, data, length);
	return ret;
}
	
int8 SessionPacket::getType()
{
	if (modified) update();
	return type;
}

uint16 SessionPacket::getLength()
{
	if (modified) update();
	return length+4;
}

uint16 SessionPacket::getDataLength()
{
	if (modified) update();
	return length;
}

uint8 *SessionPacket::getData()
{
	if (modified) update();
	if ((data==0) || (length==0)) return 0;
	uint8 *ret= new uint8[length];
	memcpy(ret, data, length);	// caller can dealloc any time
	return ret;
}

#ifdef DEBUG
void SessionPacket::print()
{
	if (modified) update();
	uint8 *p=packet();
	int l=getLength();
	for (int i=0; i<l; i++) printf("%X ",p[i]);
	cout<<"\n";
	delete p;
}
#endif

int SessionPacket::parse(SessionPacket *p)
{
	if (!p)
	{
		errno=SESSION_ERROR_NOT_ENOUGH_RESOURCES;
		return -1;
	}
	if (data) delete data;
	length=p->getLength();
	type=p->getType();
	data=p->getData();
	modified=0;			// It's a sort of update in fact...
	return 0;
}

/*
	Now implement the session packet types
*/


SessionMessagePacket::SessionMessagePacket()
{
	setType(SESSION_MESSAGE);
	setData(0,0);
	modified=1;
}

SessionMessagePacket::SessionMessagePacket(uint8 *msg, int32 msglen)
{
	setType(SESSION_MESSAGE);
	setData(msg, msglen);
	modified=1;
}

// Alias to setData
void SessionMessagePacket::setMessage(uint8 *msg, int32 msglen)
{
	setData(msg, msglen);
}



SessionRequestPacket::SessionRequestPacket(const char *calledN, const char *callingN, uint8 encode)
{
	setType(SESSION_REQUEST);
	setData(0,0);
	calledName=0;	// Parent constructor have handle
	callingName=0;	// data=0. Would it be not
	NBcalledName=0;	// 0, any code "if (data) delete data"
	NBcallingName=0;	// would end up in segfault
	if (encode)
	{
		setCalledName(calledN);
		setCallingName(callingN);
	}
	else
	{
		if (calledN)
		{
			NBcalledName=new char[strlen(calledN)+1];
			strcpy(NBcalledName, calledN);
		}
		if (callingN)
		{
			NBcallingName=new char[strlen(callingN)+1];
			strcpy(NBcallingName, callingN);
		}
	}
	modified=1;
}

void SessionRequestPacket::setCalledName(const char *name)
{
	if (name)
	{
		calledName=new char[strlen(name)+1];
		strcpy(calledName, name);
	}
	else calledName=0;
	NBcalledName=NBName(name);
//	NBcalledName=NBName("*SMBSERVER      ");
	char *n2=NBcallingName;
	if (NBcallingName==0) n2=NBName("");
	strlen(NBcalledName);
	strlen(n2);
	length=strlen(NBcalledName)+strlen(n2)+2;
	if (data) delete data;
	data = new uint8[length+4];
	strcpy((char*)(data+4), NBcalledName);
	strcpy((char*)(data+strlen(NBcalledName)+5), n2);
	if (NBcallingName==0) delete n2;
	modified=1;
}

void SessionRequestPacket::setCallingName(const char *name)
{
	if (name)
	{
		callingName=new char[strlen(name)+1];
		strcpy(callingName, name);
	}
	else callingName=0;
	NBcallingName=NBName(name);
	char *n2=NBcalledName;
	if (NBcalledName==0) n2=NBName("");
	length=strlen(NBcallingName)+strlen(n2)+2;
	if (data) delete data;
	data = new uint8[length+4];
	strcpy((char*)(data+4), n2);
	strcpy((char*)(data+strlen(n2)+5), NBcallingName);
	if (NBcalledName==0) delete n2;
	modified=1;
}

char *SessionRequestPacket::getCalledName()
{
	if (calledName==0) return 0;
	char *ret=new char[strlen(calledName)+1];
	strcpy(ret, calledName);
	return ret;
}

char *SessionRequestPacket::getCallingName()
{
	if (callingName==0) return 0;
	char *ret=new char[strlen(callingName)+1];
	strcpy(ret, callingName);
	return ret;
}

void SessionRequestPacket::update()
{
	if (!modified) return;
	if (!NBcalledName)
	{
		NBcalledName=new char[1];
		NBcalledName[0]=0;
	}
	if (!NBcallingName)
	{
		NBcallingName=new char[1];
		NBcallingName[0]=0;
	}
	length=strlen(NBcalledName)+strlen(NBcallingName)+2;
	if (data) delete data;
	data = new uint8[length];
	strcpy((char*)data, NBcalledName);
	strcpy((char*)data+strlen(NBcalledName)+1, NBcallingName);
	modified=0;
}

SessionRequestPacket::~SessionRequestPacket()
{
//	if (data) delete data; // Parent destructor will handle this
	if (NBcalledName) delete NBcalledName;
	if (NBcallingName) delete NBcallingName;
	if (calledName) delete calledName;
	if (callingName) delete callingName;
}


PositiveSessionResponsePacket::PositiveSessionResponsePacket()
{
	setType(POSITIVE_SESSION_RESPONSE);
	setData(0,0);
	modified=1;
}

NegativeSessionResponsePacket::NegativeSessionResponsePacket(uint8 err)
{
	setType(NEGATIVE_SESSION_RESPONSE);
	setData(&err,1);
	modified=1;
}

void NegativeSessionResponsePacket::setError(uint8 err)
{
	setData(&err,1);
	modified=1;
}

uint8 NegativeSessionResponsePacket::getError()
{
	if (data)
	{
		return data[0];
	}
	else
	{
		errno=SESSION_ERROR;
		return (uint8)-1;
	}
}

RetargetSessionResponsePacket::RetargetSessionResponsePacket(uint32 newIP, uint16 newPort)
{
	setType(RETARGET_SESSION_RESPONSE);
	setIP(newIP);
	setPort(newPort);
	modified=1;
}

void RetargetSessionResponsePacket::setIP(uint32 newIP)
{
	IP=newIP;
	modified=1;
//	*((uint32*)data)=BIGEND32(IP);
}

void RetargetSessionResponsePacket::setPort(uint16 newPort)
{
	port=newPort;
	modified=1;
//	*((uint16*)((int8*)data+4))=BIGEND16(port);
}

void RetargetSessionResponsePacket::update()
{
	if (!modified) return;
	if (data) delete data;
	data = new uint8[6];
	// write IP and port in big endian
	data[0]=(uint8)((IP&0xFF000000)>>24);
	data[1]=(uint8)((IP&0xFF0000)>>16);
	data[2]=(uint8)((IP&0xFF00)>>8);
	data[3]=(uint8)(IP&0xFF);
	data[4]=(uint8)((port&0xFF00)>>8);
	data[5]=(uint8)(port&0xFF);
	modified=0;
}

int RetargetSessionResponsePacket::parse(SessionPacket *p)
{
	if (SessionPacket::parse(p)==-1) return -1;
	if ((length!=6) || (data==0))
	{
		errno=SESSION_ERROR;
		return -1;
	}
	// data are in big endian
	IP=((uint32)data[0])<<24
	  |((uint32)data[1])<<16
	  |((uint32)data[2])<<8
	  |((uint32)data[3]);
	port=((uint32)data[4])<<8
	    |((uint32)data[5]);
	modified=0;
	return 0;
}


uint32 RetargetSessionResponsePacket::getIP()
{
	return IP;
}

uint16 RetargetSessionResponsePacket::getPort()
{
	return port;
}

SessionKeepAlivePacket::SessionKeepAlivePacket()
{
	setType(SESSION_KEEP_ALIVE);
	setData(0,0);
	modified=1;
}

#endif
