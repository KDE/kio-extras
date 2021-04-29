/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TESTKIOARCHIVE_H
#define TESTKIOARCHIVE_H

#include <kio/job.h>
#include <QUrl>
#include <QObject>

/**
 * This is a unit test for kio_archive
 * See also kdelibs/kio/tests/karchivetest for lower-level KTar and KZip tests.
 */
class TestKioArchive : public QObject
{
    Q_OBJECT

public:
    TestKioArchive() {}

private Q_SLOTS:
    void initTestCase();
    void testListTar();
    void testListRecursive();
    void testExtractFileFromTar();
    void testExtractSymlinkFromTar();
    void cleanupTestCase();

protected Q_SLOTS: // real slots, not tests
    void slotEntries( KIO::Job*, const KIO::UDSEntryList& lst );

private:
    QString tmpDir() const;
    QUrl tarUrl() const;
    void copyFromTar(const QUrl &url, const QString& destPath);

    QStringList m_listResult;
};

#endif
