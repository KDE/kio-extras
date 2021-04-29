/*
This file is part of KDE

 SPDX-FileCopyrightText: 2000 Waldo Bastian <bastian@kde.org>
 SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

SPDX-License-Identifier: MIT
*/

#ifndef __filter_h__
#define __filter_h__

#include <QObject>
#include <kio/slavebase.h>
#include <QUrl>

class QUrl;
class KFilterBase;

class FilterProtocol : /*public QObject, */ public KIO::SlaveBase
{
//    Q_OBJECT

public:
    FilterProtocol( const QByteArray & protocol, const QByteArray &pool, const QByteArray &app );

    void get( const QUrl &url ) override;

private:
    QUrl subURL;
    KFilterBase * filter;
};

#endif
