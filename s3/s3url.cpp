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

#include "s3url.h"

S3Url::S3Url(const QUrl &url)
    : m_url(url)
{}

bool S3Url::isRoot() const
{
    return bucketName().isEmpty() && key().isEmpty();
}

bool S3Url::isBucket() const
{
    return !bucketName().isEmpty() && key().isEmpty();
}

bool S3Url::isKey() const
{
    return !bucketName().isEmpty() && !key().isEmpty();
}

QString S3Url::bucketName() const
{
    return url().host();
}

QString S3Url::key() const
{
    return url().path();
}

QString S3Url::prefix() const
{
    if (!isKey() || key() == QLatin1String("/")) {
        return QString();
    }

    QString prefix = key().remove(0, 1);
    if (!prefix.endsWith(QLatin1Char('/'))) {
        prefix += QLatin1Char('/');
    }

    return prefix;
}

QUrl S3Url::url() const
{
    return m_url;
}

