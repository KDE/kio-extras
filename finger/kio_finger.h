
/***************************************************************************
                          kio_finger.h  -  description
                             -------------------
    begin                : Sun Aug 12 2000
    copyright            : (C) 2000 by Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef __kio_finger_h__
#define __kio_finger_h__



#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

class FingerProtocol : public QObject, public KIO::SlaveBase
{
  Q_OBJECT

public:

  FingerProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
  virtual ~FingerProtocol();

  virtual void mimetype(const KUrl& url);
  virtual void get(const KUrl& url);

private:
  KUrl                  *myURL;

  QString	        *myPerlPath;
  QString               *myFingerPath;
  QString               *myFingerPerlScript;
  QString               *myFingerCSSFile;


  void       getProgramPath();
  void       parseCommandLine(const KUrl& url);
};


#endif
