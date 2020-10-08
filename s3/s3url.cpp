/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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

