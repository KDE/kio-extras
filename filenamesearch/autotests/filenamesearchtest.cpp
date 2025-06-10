/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 Archaeopteryx Lithographica
*/

#include <QStandardPaths>
#include <QTest>
#include <QUrlQuery>

#include <KIO/ListJob>

#include <filesystem>

class FilenameSearchTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    QUrl buildSearchQuery(QByteArray searchString, QByteArray searchOptions, QString path);
    QByteArrayList doSearchQuery(QUrl url, KIO::ListJob::ListFlags listFlags);

    void initTestCase();

    void filenameContent_data();
    void filenameContent();

    void optionCase_data();
    void optionCase();

    void folderTree_data();
    void folderTree();

    void folderTreeSymlinks_data();
    void folderTreeSymlinks();

    void hiddenFiles_data();
    void hiddenFiles();

    void hiddenFolders_data();
    void hiddenFolders();

    void hiddenFilesAndFolders_data();
    void hiddenFilesAndFolders();

    void cleanupTestCase();

private:
    QString m_workingFolder = "";
};

namespace
{

void addColumns()
{
    QTest::addColumn<QByteArray>("searchOptions");
    QTest::addColumn<QByteArray>("searchString");
    QTest::addColumn<QByteArrayList>("expectedFiles");
};

auto addRow = [](const char *name, const QByteArray &searchOptions, const QByteArray &searchString, const QByteArrayList &expectedFiles) {
    QTest::addRow("%s", name) << searchOptions << searchString << expectedFiles;
};
}

QUrl FilenameSearchTest::buildSearchQuery(QByteArray searchString, QByteArray searchOptions, QString path)
{
    //  Encode the testfile directory

    QUrl searchFrom;
    searchFrom.setPath(QString(QUrl::toPercentEncoding(QStringLiteral("file://") + path)), QUrl::StrictMode);

    //  Parse the search options, create a map and build the query
    //  Add a "src=internal" option to all queries

    QUrlQuery queryOptions(searchOptions);
    QList<QPair<QString, QString>> map = {{QStringLiteral("search"), QString(searchString)}};
    map.append({{QStringLiteral("src"), QStringLiteral("internal")}});
    map.append({{QStringLiteral("url"), searchFrom.toString()}});
    map.append(queryOptions.queryItems());

    QUrlQuery urlQuery;
    urlQuery.setQueryItems(map);

    //  Build the search Url

    QUrl url;
    url.setScheme("filenamesearch");
    url.setQuery(urlQuery);

    return (url);
}

QByteArrayList FilenameSearchTest::doSearchQuery(QUrl url, KIO::ListJob::ListFlags listFlags)
{
    QByteArrayList results;

    KIO::ListJob *listJob = KIO::listDir(url, KIO::HideProgressInfo, listFlags);

    connect(this, &QObject::destroyed, listJob, [listJob]() {
        listJob->kill();
    });

    //  ListJob with IncludeHidden will read the match for "."

    connect(listJob, &KIO::ListJob::entries, this, [&](KJob *, const KIO::UDSEntryList &list) {
        if (listJob->error()) {
            qWarning() << "Searching failed:" << listJob->errorText();
            return;
        }

        for (auto entry : list) {
            const QByteArray fileName = entry.stringValue(KIO::UDSEntry::UDS_NAME).toUtf8();
            if (fileName != QByteArrayLiteral(".")) {
                results += fileName;
            }
        }
    });

    listJob->exec();

    std::sort(results.begin(), results.end());
    return results;
}

void FilenameSearchTest::initTestCase()
{
    const QString sourcePath = QFINDTESTDATA("data/folderTree");
    QString destinationPath = sourcePath;
    destinationPath.replace("/folderTree", "/folderTreeSymlinks");

    if (sourcePath.indexOf(QStringLiteral("filenamesearch/autotests/data")) < 0) {
        qInfo() << "Cannot find location of \"data/folderTree\"";
        m_workingFolder = "";
        return;
    } else {
        QDir destination(destinationPath);
        if (destination.removeRecursively()) {
            m_workingFolder = "";
        }
    }

    //  Programatically add a .bak file to the test data, a static file fails an audit
    //  when being uploaded to invent. Use QFile::copy

    QFile::remove(sourcePath + QStringLiteral("/alice1.txt.bak"));

    if (!QFile::copy(sourcePath + QStringLiteral("/alice1.txt"), sourcePath + QStringLiteral("/alice1.txt.bak"))) {
        qInfo() << "Failed copying testfile alice1.txt to alice1.txt.bak";
    }

    //  Use std::filesystem::copy to copy folderTree as there's not a Qt equivalent.

    std::error_code result;
    std::filesystem::path sourceStdPath(sourcePath.toStdString());
    std::filesystem::path destinationStdPath(destinationPath.toStdString());

    std::filesystem::copy(sourceStdPath, destinationStdPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_symlinks, result);

    if (result.value() != 0) {
        qWarning() << "Copy failed" << result.message();
        m_workingFolder = "";
    } else {
        m_workingFolder = destinationPath;
        const QString childParentLink = destinationPath + QStringLiteral("/child/parent");
        const QString grandchildParentLink = destinationPath + QStringLiteral("/child/grandchild/parent");
        const QString grandchildGrandparentLink = destinationPath + QStringLiteral("/child/grandchild/grandparent");
        QFile::link(QStringLiteral("../"), childParentLink);
        QFile::link(QStringLiteral("../"), grandchildParentLink);
        QFile::link(QStringLiteral("../../"), grandchildGrandparentLink);

        const QString hiddenChildParentLink = destinationPath + QStringLiteral("/.child/parent");
        QFile::link(QStringLiteral("../"), hiddenChildParentLink);
    }
};

void FilenameSearchTest::cleanupTestCase()
{
    const QString sourcePath = QFINDTESTDATA("data/folderTree");
    if (sourcePath.indexOf(QStringLiteral("filenamesearch/autotests/data")) > 0) {
        QFile::remove(sourcePath + QStringLiteral("/alice1.txt.bak"));
    }

    if (m_workingFolder.indexOf(QStringLiteral("filenamesearch/autotests/data")) >= 0) {
        QDir dir(m_workingFolder);
        if (!dir.removeRecursively()) {
            qWarning() << "Failed to clean:" << m_workingFolder;
        }
    }
};

void FilenameSearchTest::filenameContent_data()
{
    addColumns();

    addRow("No Filename Match", QByteArrayLiteral(""), QByteArrayLiteral("curiouser"), {});
    addRow("Filename Match", QByteArrayLiteral(""), QByteArrayLiteral("alice1"), {QByteArrayLiteral("alice1.txt")});

    addRow("No Content Match", QByteArrayLiteral("checkContent=yes"), QByteArrayLiteral("wonderland"), {});
    addRow("Content Match", QByteArrayLiteral("checkContent=yes"), QByteArrayLiteral("curiouser"), {QByteArrayLiteral("alice2.txt")});

    addRow("Filename and Content Match",
           QByteArrayLiteral("checkContent=yes"),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice2.txt")});
}

void FilenameSearchTest::filenameContent()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/filename-content");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QCOMPARE(results, expectedFiles);
}

//  TODO: the optionCase test relies on filenameContent, ought to have an option/query that does not depend on test files.

void FilenameSearchTest::optionCase_data()
{
    addColumns();

    addRow("No Match", QByteArrayLiteral("checkContent=no"), QByteArrayLiteral("curiouser"), {});
    addRow("Lower Case", QByteArrayLiteral("checkContent=yes"), QByteArrayLiteral("curiouser"), {QByteArrayLiteral("alice2.txt")});
    addRow("Mixed Case", QByteArrayLiteral("checkContent=Yes"), QByteArrayLiteral("curiouser"), {QByteArrayLiteral("alice2.txt")});
    addRow("Upper Case", QByteArrayLiteral("checkContent=YES"), QByteArrayLiteral("curiouser"), {QByteArrayLiteral("alice2.txt")});
}

void FilenameSearchTest::optionCase()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/filename-content");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QCOMPARE(results, expectedFiles);
}

//  Search including hidden files, to catch the .bak (that may be considered hidden)

void FilenameSearchTest::folderTree_data()
{
    addColumns();

    addRow("Filename Match",
           QByteArray(""),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice1.txt.bak"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});

    addRow("Content Match",
           QByteArray("checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchTest::folderTree()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/folderTree");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {KIO::ListJob::ListFlag::IncludeHidden});

    QCOMPARE(results, expectedFiles);
}

//  A filename search for "alice" finds the .bak, backup file,
//  however a content search does not consider .bak text/plain.

void FilenameSearchTest::folderTreeSymlinks_data()
{
    addColumns();

    addRow("Filename Match",
           QByteArray(""),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice1.txt.bak"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});

    addRow("Content Match",
           QByteArray("checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchTest::folderTreeSymlinks()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/folderTreeSymlinks/child");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchTest::hiddenFiles_data()
{
    addColumns();

    addRow("Filename Match",
           QByteArray("includeHidden=files"),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral(".alice2.txt"), QByteArrayLiteral("alice1.txt")});

    addRow("Content Match",
           QByteArray("includeHidden=files&checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral(".alice2.txt"), QByteArrayLiteral("alice1.txt")});
}

void FilenameSearchTest::hiddenFiles()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/hiddenFilesAndFolders");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {KIO::ListJob::ListFlag::IncludeHidden});

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchTest::hiddenFolders_data()
{
    addColumns();

    addRow("Filename Match",
           QByteArray("includeHidden=folders"),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});

    addRow("Content Match",
           QByteArray("includeHidden=folders&checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchTest::hiddenFolders()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/hiddenFilesAndFolders");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {KIO::ListJob::ListFlag::IncludeHidden});

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchTest::hiddenFilesAndFolders_data()
{
    addColumns();

    addRow("Filename Match",
           QByteArray("includeHidden=yes"),
           QByteArrayLiteral("alice"),
           {QByteArrayLiteral(".alice2.txt"), QByteArrayLiteral(".alice4.txt"), QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});

    addRow("Content Match",
           QByteArray("includeHidden=yes&checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral(".alice2.txt"), QByteArrayLiteral(".alice4.txt"), QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchTest::hiddenFilesAndFolders()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/hiddenFilesAndFolders");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), KIO::ListJob::ListFlag::IncludeHidden);

    QCOMPARE(results, expectedFiles);
}

QTEST_MAIN(FilenameSearchTest)

#include "filenamesearchtest.moc"
