/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef KIO_CGI_H
#define KIO_CGI_H

#include <qobject.h>

#include <kio/slavebase.h>

class KProcess;

/*!
  This class implements an ioslave for viewing CGI script output without the
  need to run a web server.
*/
class CgiProtocol : public KIO::SlaveBase
{
  public:
    CgiProtocol( const QByteArray &pool, const QByteArray &app );
    virtual ~CgiProtocol();

    virtual void get( const KUrl& url );

//    virtual void mimetype( const KUrl& url );

  protected:
//    QCString errorMessage();

  private:
    QStringList mCgiPaths;
};

#endif
