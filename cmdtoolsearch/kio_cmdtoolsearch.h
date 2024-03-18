/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/ForwardingWorkerBase>

#include <QUrl>

#include <queue>
#include <set>

class CmdToolSearchProtocol : public KIO::ForwardingWorkerBase
{
    Q_OBJECT

public:
    CmdToolSearchProtocol(const QByteArray &pool, const QByteArray &app);

    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult listDir(const QUrl &url) override;

protected:
    bool rewriteUrl(const QUrl &url, QUrl &newURL) override;
    void adjustUDSEntry(KIO::UDSEntry &entry, UDSEntryCreationMode creationMode) const override;

private:
    void listRootEntry();
    KIO::WorkerResult runKioFileNameSearch(const QUrl &url);
};
