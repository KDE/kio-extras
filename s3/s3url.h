/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
    const QUrl m_url;
};

#endif // S3URL_H
