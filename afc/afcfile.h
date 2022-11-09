/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KIO/WorkerBase>

#include "afcclient.h"
#include "afcfilereader.h"

#include <optional>

class AfcFile
{
public:
    AfcFile(const AfcClient::Ptr &client, const QString &path);
    AfcFile(AfcFile &&other) Q_DECL_NOEXCEPT;
    ~AfcFile();

    AfcClient::Ptr client() const;
    QString path() const;

    KIO::WorkerResult open(QIODevice::OpenMode mode);
    KIO::WorkerResult seek(KIO::filesize_t offset);
    KIO::WorkerResult truncate(KIO::filesize_t length);
    KIO::WorkerResult write(const QByteArray &data, uint32_t &bytesWritten);
    KIO::WorkerResult close();

    AfcFileReader reader() const;

private:
    AfcClient::Ptr m_client;
    QString m_path;

    std::optional<uint64_t> m_handle;

    Q_DISABLE_COPY(AfcFile)
};


