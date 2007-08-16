/*
This file is part of KDE

 Copyright (C) 2000 Waldo Bastian (bastian@kde.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __filter_h__
#define __filter_h__

#include <QObject>
#include <kio/global.h>
#include <kio/slavebase.h>
class KFilterBase;
class FilterProtocol : public QObject, public KIO::SlaveBase
{
  Q_OBJECT

public:
  FilterProtocol( const QByteArray & protocol, const QByteArray &pool, const QByteArray &app );

  virtual void get( const KUrl &url );
  virtual void put( const KUrl &url, int _mode, bool _overwrite,
                    bool _resume );
  virtual void setSubURL(const KUrl &url);

private:
  KUrl subURL;
  KFilterBase * filter;
};

#endif
