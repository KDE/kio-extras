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
 *
 * ?src=[Internal|External]
 *
 * The available options depend on whether an internal, native, search is being
 * done or filenamesearch invokes the external search script kio_filenamesearch_grep.
 *
 * The "src" query options specifies whether to use the internal or external search.
 * If "src=internal" is specified, the internal search code is used. A "src=external"
 * the external script is used.
 *
 * If the "src" is not specified, the external search script is used if present and the
 * search falls back to using the internal search if it is not.
 *
 * For the internal search:
 *
 * ?checkContent=[Yes|No]
 *
 * If the query item "checkContent" is set to "yes", the content as well as
 * the filename of files with a text MimeType will be searched for the given pattern.
 *
 * ?includeHidden=[Yes|Files|Folders|FilesAndFolders]
 *
 * The "includeHidden" query item specifies whether the internal search should
 * read hidden files and/or navigate down into hidden folders.
 *
 * If "includeHidden=yes" or "includeHidden=FilesAndFolders", both hidden files and
 * hidden folders are searched. If "includeHidden=Files", just hidden files are
 * searched without exploring hidden folders. If "includeHidden=Folders", the
 * search recursively explores hidden folders but does not read hidden files.
 *
 * ?syntax=[Phrase|Regex|Wordlist]
 *
 * The "syntax" query item specified how the search string/expression should be handled.
 *
 * If "syntax=phrase", the search query is treated as a string, matching whitespace
 * flexibly. If "syntax=regex", the query is treated as a regular expression.
 * If "syntax=wordlist", the query is treated as a set of words (and quoted text as
 * phrases), split by spaces, and an "all words" match is done.
 *
 * The default behaviour is "syntax=regex".
 *
 * "syntax=phrase" and "syntax=regex" also work for the external search, "syntax=wordlist"
 * does not.
 */

namespace FileNameSearch
{

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
    enum class SearchOption {
        SearchFileName = 0x1,
        SearchContent = 0x2,
        IncludeHiddenFiles = 0x10,
        IncludeHiddenFolders = 0x20,
        IncludeHidden = 0x30, // IncludeHiddenFiles | IncludeHiddenFolders
        CaseSensitive = 0x40, // For future use
    };

    Q_DECLARE_FLAGS(SearchOptions, SearchOption)

    enum class SearchSrc {
        Internal = 0x1,
        External = 0x2,
        ExternalThenInternal = 0x3, // External with Fallback;
    };

    Q_DECLARE_FLAGS(SearchSrcs, SearchSrc)

    enum class SearchSyntax {
        //  Literal,
        Phrase,
        WordList,
        //  Glob,
        Regex,
    };

    void listRootEntry();
    void searchDir(const QUrl &dirUrl,
                   const QHash<QRegularExpression, int> &regexHash,
                   const SearchOptions options,
                   std::set<QString> &iteratedDirs,
                   std::queue<QUrl> &pendingDirs);
    bool match(const KIO::UDSEntry &entry, QHash<QRegularExpression, int> regexHash, const SearchOptions options);

    QStringList splitWordList(const QString pattern);
    QString escapeApostrophes(const QString pattern);

    SearchOptions parseSearchOptions(const QString optionContent, const QString optionHidden, const SearchOption defaultOptions);
    SearchSrcs parseSearchSrc(const QString optionSrc, const SearchSrc defaultSrcs);
    SearchSyntax parseSearchSyntax(const QString optionSyntax, const SearchSyntax defaultSyntax);

#if !defined(Q_OS_WIN32)

    /**
     * @brief Searches the directory using external tools if available.
     * @return ERR_UNSUPPORTED_ACTION if the external tool is not available. Otherwise, the result of the search.
     */
    KIO::WorkerResult searchDirWithExternalTool(const QUrl &dirUrl, const QRegularExpression &regex);

#endif // !defined(Q_OS_WIN32)
};

} // namespace FileNameSearch

#endif
