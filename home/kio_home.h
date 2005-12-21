/* This file is part of the KDE project
   Copyright (c) 2005 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KIO_HOME_H
#define KIO_HOME_H

#include <kio/forwardingslavebase.h>
#include "homeimpl.h"

class HomeProtocol : public KIO::ForwardingSlaveBase
{
public:
	HomeProtocol(const QByteArray &protocol, const QByteArray &pool,
	             const QByteArray &app);
	virtual ~HomeProtocol();

	virtual bool rewriteURL(const KURL &url, KURL &newUrl);
	
	virtual void listDir(const KURL &url);
	virtual void stat(const KURL &url);

private:
	void listRoot();
	
	HomeImpl m_impl;
};

#endif
