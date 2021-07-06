/*
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SMBCDISCOVERER_H
#define SMBCDISCOVERER_H

#include <QObject>

#include "discovery.h"
#include "kio_smb.h"

class QEventLoop;

class SMBCDiscovery : public Discovery
{
public:
    SMBCDiscovery(const UDSEntry &entry);
    QString udsName() const override;
    KIO::UDSEntry toEntry() const override;

protected:
    UDSEntry m_entry;

private:
    const QString m_name;
};

class SMBCDiscoverer : public QObject, public Discoverer
{
    Q_OBJECT
public:
    SMBCDiscoverer(const SMBUrl &url, QEventLoop *discoverNext, SMBSlave *slave);
    ~SMBCDiscoverer() override;

    void start() override;
    bool isFinished() const override;

    bool dirWasRoot() const;
    int error() const;

Q_SIGNALS:
    void newDiscovery(Discovery::Ptr discovery) override;
    void finished() override;

private Q_SLOTS:
    /**
     * Process one dirent, queue a new loop event and return.
     * @see customEvent
     * @see queue
     */
    void discoverNext();

protected:
    void customEvent(QEvent *event) override;

private:
    void stop() override;

    /**
     * readdirplus2 based file_info looping
     * @returns whether a file info was looped on
     */
    bool discoverNextFileInfo();

    /// init discoverer (calls opendir)
    void init();

    /// queue new loop run
    inline void queue();

    SMBUrl m_url;
    QEventLoop *m_loop = nullptr;
    SMBSlave *m_slave = nullptr;
    bool m_finished = false;
    int m_error = 0;
    bool m_dirWasRoot = true;
    int m_dirFd = -1;
};

#endif // SMBCDISCOVERER_H
