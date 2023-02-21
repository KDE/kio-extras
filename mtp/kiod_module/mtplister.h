// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#pragma once

#include <span>

#include <QObject>

#include "kmtpfile.h"
#include "memory.h"

class MTPStorage;

class MTPLister : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmtp.Lister")
public:
    explicit MTPLister(std::unique_ptr<uint32_t> children, int childrenCount, LIBMTP_mtpdevice_t *device, const QString &path, MTPStorage *parent = nullptr);

public Q_SLOTS:
    void run();
    void abort();

Q_SIGNALS:
    void entry(const KMTPFile &file);
    void finished();

private:
    LIBMTP_mtpdevice_t *const m_device;
    const QString m_path;
    const std::unique_ptr<uint32_t> m_childrenOwner;
    const std::span<uint32_t> m_children;
    decltype(m_children)::iterator m_it;
};
