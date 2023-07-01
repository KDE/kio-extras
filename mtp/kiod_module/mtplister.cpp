// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include "mtplister.h"

#include "listeradaptor.h"
#include "mtpfile.h"
#include "mtpstorage.h"

MTPLister::MTPLister(std::unique_ptr<uint32_t> children, int childrenCount, LIBMTP_mtpdevice_t *device, const QString &path, MTPStorage *parent)
    : QObject(parent)
    , m_device(device)
    , m_path(path)
    , m_childrenOwner(std::move(children))
    , m_children(m_childrenOwner.get(), childrenCount)
    , m_it(m_children.begin())
{
    new ListerAdaptor(this);
}

void MTPLister::run()
{
    if (m_it == m_children.end()) {
        Q_EMIT finished();
        deleteLater();
        return;
    }

    std::unique_ptr<LIBMTP_file_t> file(LIBMTP_Get_Filemetadata(m_device, *m_it));
    if (file) { // file may have disappeared in the time between the id listing and metadata retrieval
        Q_EMIT entry(createKMTPFile(file));
    }

    ++m_it;
    QMetaObject::invokeMethod(this, &MTPLister::run, Qt::QueuedConnection);
}

void MTPLister::abort()
{
    m_it = m_children.end();
    QMetaObject::invokeMethod(this, &MTPLister::run, Qt::QueuedConnection);
}

#include "moc_mtplister.cpp"
