/*
 *  Copyright (C) 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef S3URL_H
#define S3URL_H

#include <QUrl>

class S3Url
{
public:
    explicit S3Url(const QUrl &url);

    bool isRoot() const;
    bool isBucket() const;
    bool isKey() const;
    QString bucketName() const;
    QString key() const;
    QString prefix() const;
    QUrl url() const;

private:
    QUrl m_url;
};

#endif // S3URL_H
