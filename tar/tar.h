/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef _TAR_H
#define _TAR_H "$Id$"

#include <kio/slavebase.h>

class TARProtocol : public KIO::SlaveBase
{
public:
  TARProtocol( KIO::Connection *connection = 0 );
  virtual ~TARProtocol();

  virtual void listDir( const QString & path );
  virtual void stat( const QString & path );

  virtual void get( const QString& path, const QString& query, bool reload );

  /*
  void jobData( void *_p, int _len );
  void jobDataEnd();
  void filterData( void *_p, int _len );
  */

 protected:

};

#endif
