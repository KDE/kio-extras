/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcfile.h"

#include "afc_debug.h"

#include "afcutils.h"

using namespace KIO;

AfcFile::AfcFile(const AfcClient::Ptr &client, const QString &path)
    : m_client(client)
    , m_path(path)
{
}

AfcFile::AfcFile(AfcFile &&other) Q_DECL_NOEXCEPT : m_client(std::move(other.m_client)), m_path(other.m_path), m_handle(other.m_handle)
{
    other.m_handle.reset();
}

AfcFile::~AfcFile()
{
    if (m_handle) {
        close();
    }
}

AfcClient::Ptr AfcFile::client() const
{
    return m_client;
}

QString AfcFile::path() const
{
    return m_path;
}

WorkerResult AfcFile::open(QIODevice::OpenMode mode)
{
    Q_ASSERT(!m_handle);

    afc_file_mode_t fileMode = static_cast<afc_file_mode_t>(0);

    if (mode == QIODevice::ReadOnly) {
        fileMode = AFC_FOPEN_RDONLY;
    } else if (mode == QIODevice::WriteOnly) {
        fileMode = AFC_FOPEN_WRONLY;
    } else if (mode == QIODevice::ReadWrite) {
        fileMode = AFC_FOPEN_RW;
    } else if (mode == (QIODevice::ReadWrite | QIODevice::Truncate)) {
        fileMode = AFC_FOPEN_WR;
    } else if (mode == QIODevice::Append || mode == (QIODevice::Append | QIODevice::WriteOnly)) {
        fileMode = AFC_FOPEN_APPEND;
    } else if (mode == (QIODevice::Append | QIODevice::ReadWrite)) {
        fileMode = AFC_FOPEN_RDAPPEND;
    }

    if (!fileMode) {
        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, QString::number(mode));
    }

    uint64_t handle = 0;
    const auto ret = afc_file_open(m_client->internalClient(), m_path.toLocal8Bit().constData(), fileMode, &handle);

    const WorkerResult result = AfcUtils::Result::from(ret);
    if (result.success()) {
        m_handle = handle;
    }
    return result;
}

WorkerResult AfcFile::seek(filesize_t offset)
{
    Q_ASSERT(m_handle);
    const auto ret = afc_file_seek(m_client->internalClient(), m_handle.value(), offset, SEEK_SET);
    return AfcUtils::Result::from(ret);
}

WorkerResult AfcFile::truncate(filesize_t length)
{
    Q_ASSERT(m_handle);
    const auto ret = afc_file_truncate(m_client->internalClient(), m_handle.value(), length);
    return AfcUtils::Result::from(ret);
}

WorkerResult AfcFile::write(const QByteArray &data, uint32_t &bytesWritten)
{
    Q_ASSERT(m_handle);
    const auto ret = afc_file_write(m_client->internalClient(), m_handle.value(), data.constData(), data.size(), &bytesWritten);
    return AfcUtils::Result::from(ret);
}

WorkerResult AfcFile::close()
{
    Q_ASSERT(m_handle);

    const auto ret = afc_file_close(m_client->internalClient(), m_handle.value());

    const WorkerResult result = AfcUtils::Result::from(ret);
    if (result.success()) {
        m_handle.reset();
    }
    return result;
}

AfcFileReader AfcFile::reader() const
{
    Q_ASSERT(m_handle);

    AfcFileReader reader(m_client, m_handle.value());
    return reader;
}
