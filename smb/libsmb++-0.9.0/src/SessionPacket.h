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

#ifndef __SESSIONPACKET_H__
#define __SESSIONPACKET_H__

#include "defines.h"
#ifndef USE_SAMBA

#include "NetBIOSPacket.h"
#include "SessionCodes.h"

// class SessionPacket to define and handle the differents kinds of packets

class SessionPacket : public NetBIOSPacket
{
protected:
	int8 type;
	int32 length;// do not worry about the size of this field
	uint8 *data;  // it will be OK when the packet is returned
public:
	SessionPacket(int8 t=0,  uint8* d=0, int32 l=0);
	~SessionPacket();
	void setType(int8 t=0);
	void setData(uint8* d=0, int32 l=0);
	// virtuals of the parent
	uint8* packet();
	int8 getType();
	uint16 getLength();
	uint16 getDataLength(); // Get the real length of the data
	uint8 *getData();
	// prints the packet
	void print();
	// parse a SessionPacket into our structure
	// do just a copy at this level, but subclasses
	// will implement they own version.
	// It's really useful with SessionIO::receive()
	//   1. received_packet->getType()
	//   2. packet_of_this_type->parse()
	// return -1 on error and errorcode in errno
	virtual int parse(SessionPacket *p);
};


class SessionMessagePacket : public SessionPacket
{
public:
	SessionMessagePacket();
	SessionMessagePacket(uint8 *msg, int32 msglen);  // set a message now
	void setMessage(uint8 *msg, int32 msglen);  // alias to setData
};

class SessionRequestPacket : public SessionPacket
{
protected:
	char *NBcalledName, *NBcallingName;  // NetBIOS Names
	char *calledName, *callingName;  // ASCIIZ Names
	void update();	// Need to update data when names are changed
public:
	// The calling name should be ours, but...
	// encode indicates if we should convert names into NetBIOS format
	SessionRequestPacket(const char *calledN="", const char *callingN="", uint8 encode=0);
	~SessionRequestPacket();
	void setCalledName(const char *name);	// Will be converted into NetBios
	void setCallingName(const char *name); // name
	char *getCalledName();		// Return ASCIIZ name, not NetBios
	char *getCallingName();		// Return ASCIIZ name, not NetBios
};

class PositiveSessionResponsePacket : public SessionPacket
{
public:
	PositiveSessionResponsePacket();
};

class NegativeSessionResponsePacket : public SessionPacket
{
public:
	NegativeSessionResponsePacket(uint8 err=SESSION_ERROR);
	void setError(uint8 err=SESSION_ERROR);
	uint8 getError();
};

class RetargetSessionResponsePacket : public SessionPacket
{
protected:
	uint32 IP, port;
	void update();
public:
	RetargetSessionResponsePacket(uint32 newIP, uint16 newPort);
	void setIP(uint32 newIP);
	uint32 getIP();
	void setPort(uint16 newPort);
	uint16 getPort();
	int parse(SessionPacket *p);
};

class SessionKeepAlivePacket : public SessionPacket
{
public:
	SessionKeepAlivePacket();
};

#endif
#endif //__SESSIONPACKET_H__
