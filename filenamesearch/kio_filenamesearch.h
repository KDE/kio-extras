/*
 *   SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIO_FILENAMESEARCH_H
#define KIO_FILENAMESEARCH_H

#include <KIO/WorkerBase>

#include <QFlags>
#include <QUrl>

#include <queue>
#include <set>

/**
 * @brief Lists files that match a specific search pattern.
 *
 * For example, an application could create a url:
 * filenamesearch:?search=sometext&url=file:///home/foo/bar&title=Query Results from 'sometext'
 *
 * - The pattern to search for, @c sometext is the value of the "search" query
 * item of the URL
 * - The directory where the search is performed, @c file:///home/foo/bar, is the value
 * of the "url" query item.
 *
 * By default the files with names matching the search pattern are listed.
 * Alternatively if the query item "checkContent" is set to "yes", the contents
 * of files with a text MimeType will be searched for the given pattern and
 * the matching files will be listed.
 */
class FileNameSearchProtocol : public QObject, public KIO::WorkerBase
{
    Q_OBJECT

public:
    FileNameSearchProtocol(const QByteArray &pool, const QByteArray &app);
    ~FileNameSearchProtocol() override;

    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult listDir(const QUrl &url) override;

private:
    //  Define the various search flags for internal use.
    enum SearchOption {
        SearchFileName = 0x1,
        SearchContent = 0x2,
        IncludeHiddenFiles = 0x10,
        IncludeHiddenFolders = 0x20,
        IncludeHidden = 0x30, // IncludeHiddenFiles | IncludeHiddenFolders
        CaseSensitive = 0x40, // For future use
    };

    Q_DECLARE_FLAGS(SearchOptions, SearchOption)

    void listRootEntry();
    void
    searchDir(const QUrl &dirUrl, const QRegularExpression &regex, const SearchOptions options, std::set<QString> &iteratedDirs, std::queue<QUrl> &pendingDirs);
    bool match(const KIO::UDSEntry &entry, const QRegularExpression &regex, const SearchOptions options);

#if !defined(Q_OS_WIN32)

    /**
     * @brief Searches the directory using external tools if available.
     * @return ERR_UNSUPPORTED_ACTION if the external tool is not available. Otherwise, the result of the search.
     */
    KIO::WorkerResult searchDirWithExternalTool(const QUrl &dirUrl, const QRegularExpression &regex);

#endif // !defined(Q_OS_WIN32)
};

#endif
