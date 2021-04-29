/*  This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef __kio_about_h__
#define __kio_about_h__

#include <QByteArray>
#include <QUrl>

#include <kio/global.h>
#include <kio/slavebase.h>


class AboutProtocol : public KIO::SlaveBase
{
public:
    AboutProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~AboutProtocol() override;

    void get(const QUrl& url) override;
    void mimetype(const QUrl& url) override;
};

#endif
