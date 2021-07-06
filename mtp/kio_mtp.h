/*
 *  Main implementation for KIO-MTP
 *  SPDX-FileCopyrightText: 2012 Philipp Schmidt <philschmidt@gmx.net>
 *  SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIO_MTP_H
#define KIO_MTP_H

#include <kio/global.h>
#include <kio/slavebase.h>
#include <KLocalizedString>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>


// #include <QCache>

#include <QUrl>

#define MAX_XFER_BUF_SIZE 16348
#define KIO_MTP 7000
#include <kmtpdinterface.h>

class KMTPDeviceInterface;
class KMTPStorageInterface;
class KMTPFile;

using namespace KIO;

class MTPSlave : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    /*
     * Overwritten KIO-functions, see "kio_mtp.cpp"
     */
    MTPSlave(const QByteArray &pool, const QByteArray &app);
    ~MTPSlave() override;

    void listDir(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void mimetype(const QUrl &url) override;
    void get(const QUrl &url) override;
    void put(const QUrl &url, int, JobFlags flags) override;
    void copy(const QUrl &src, const QUrl &dest, int, JobFlags flags) override;
    void mkdir(const QUrl &url, int) override;
    void del(const QUrl &url, bool) override;
    void rename(const QUrl &src, const QUrl &dest, JobFlags flags) override;

// private Q_SLOTS:
//
//     void test();

protected:
    void virtual_hook(int id, void *data) override;

private:
    /**
     * Check if it is a valid url or an udi.
     *
     * @param url The url to checkUrl
     * @param redirect If udi= should be redirected or just return false
     * @return 0 if valid, 1 if udi and redirected, 2 if udi but invalid device, -1 else
     */
    int checkUrl(const QUrl &url, bool redirect = true);

    void fileSystemFreeSpace(const QUrl &url);

    /**
     * @brief Waits for a pending copy operation to finish while updating its progress.
     * @param storage
     * @return The result of the operation, usually 0 if valid
     */
    int waitForCopyOperation(const KMTPStorageInterface *storage);

    KMTPDInterface m_kmtpDaemon;
};

#endif // KIO_MTP_H
