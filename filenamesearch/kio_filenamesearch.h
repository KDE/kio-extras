/*
 *   SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIO_FILENAMESEARCH_H
#define KIO_FILENAMESEARCH_H

#include <kio/slavebase.h>

#include <functional>

#include <QLoggingCategory>

class QUrl;
class QRegularExpression;
class KFileItem;

Q_DECLARE_LOGGING_CATEGORY(KIO_FILENAMESEARCH)

/**
 * @brief Lists files where the filename matches do a given query.
 *
 * The query is defined as part of the "search" query item of the URL.
 * The directory where the searching is started is defined in the "url" query
 * item. If the query item "checkContent" is set to "yes", all files with
 * a text MIME type will be checked for the content.
 */
class FileNameSearchProtocol : public KIO::SlaveBase
{
public:
    FileNameSearchProtocol(const QByteArray &pool, const QByteArray &app);
    ~FileNameSearchProtocol() override;

    void stat(const QUrl& url) override;
    void listDir(const QUrl &url) override;

private:
    void searchDirectory(const QUrl &directory,
                         const std::function<bool(const KFileItem &)> &itemValidator,
                         QSet<QString> &iteratedDirs);

    /**
     * @return True, if the \a pattern is part of the file \a fileName.
     */
    static bool contentContainsPattern(const QUrl &fileName, const QRegularExpression &pattern);
};

#endif
