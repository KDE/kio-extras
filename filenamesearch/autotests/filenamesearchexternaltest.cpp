/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 Archaeopteryx Lithographica
*/

#include <QProcess>
#include <QStandardPaths>
#include <QTest>
#include <QUrlQuery>

#include <KIO/ListJob>

#include <filesystem>

class FilenameSearchExternalTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    QUrl buildSearchQuery(QByteArray searchString, QByteArray searchOptions, QString path);
    QByteArrayList doSearchQuery(QUrl url, KIO::ListJob::ListFlags listFlags);
    bool externalSearchInstalled(const QString searchScript);

    void initTestCase();

    void stringMatch_data();
    void stringMatch();

    void phraseMatch_data();
    void phraseMatch();

    void regexMatch_data();
    void regexMatch();

    void wordListMatch_data();
    void wordListMatch();

    void filenameContent_data();
    void filenameContent();

    void folderTree_data();
    void folderTree();

    void folderTreeSymlinks_data();
    void folderTreeSymlinks();

    void hiddenFilesAndFolders_data();
    void hiddenFilesAndFolders();

    void cleanupTestCase();

private:
    QString m_workingFolder = "";
    QString m_programName = "";
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

QUrl FilenameSearchExternalTest::buildSearchQuery(QByteArray searchString, QByteArray searchOptions, QString path)
{
    //  Encode the testfile directory

    QUrl searchFrom;
    searchFrom.setPath(QString(QUrl::toPercentEncoding(QStringLiteral("file://") + path)), QUrl::StrictMode);

    //  Parse the search options, create a map and build the query
    //  Add a "src=external" option to all queries

    QUrlQuery queryOptions(searchOptions);
    QList<QPair<QString, QString>> map = {{QStringLiteral("search"), QString(searchString)}};
    map.append({{QStringLiteral("src"), QStringLiteral("external")}});
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

QByteArrayList FilenameSearchExternalTest::doSearchQuery(QUrl url, KIO::ListJob::ListFlags listFlags)
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

bool FilenameSearchExternalTest::externalSearchInstalled(const QString searchScript)
{
    bool externalScript = false;
    qInfo() << searchScript;

    if (!searchScript.isEmpty()) {
        QProcess process;
        process.setProgram(searchScript);
        process.setArguments({QStringLiteral("--check")});

        process.start(QIODeviceBase::ReadWrite | QIODeviceBase::Unbuffered);
        if (process.waitForStarted(5000)) {
            process.closeWriteChannel();
            process.waitForFinished(5000);
            externalScript = (process.exitCode() == 0);
        } else {
            qWarning() << searchScript << "failed to start:" << process.errorString();
        }
    }

    return (externalScript);
}

void FilenameSearchExternalTest::initTestCase()
{
    m_programName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_filenamesearch/kio-filenamesearch-grep"));
    if (!externalSearchInstalled(m_programName)) {
        QSKIP("External script not found");
        return;
    }

    //  Find the source folder for the test data, set up a destination folder
    //  that will contain the symlinked folders.

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
    //  when being uploaded to invent.
    //  Similarly the symlinks have to be created programatically to avoid trouble uploading.

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

void FilenameSearchExternalTest::cleanupTestCase()
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

//  Tests of the external script (assuming ripgrep) explicitly state "syntax=regex"

void FilenameSearchExternalTest::stringMatch_data()
{
    addColumns();

    addRow("No Filename Match", QByteArray(""), QByteArrayLiteral("Cheshire"), {});
    addRow("Filename Match", QByteArray(""), QByteArrayLiteral("alice.txt"), {QByteArrayLiteral("alice.txt")});

    addRow("No Content Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("Cheshire"), {});
    addRow("Content Word Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("Wonderland"), {QByteArrayLiteral("alice.txt")});
    addRow("Content Leading Substring Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("Wonder"), {QByteArrayLiteral("alice.txt")});
    addRow("Content Trailing Substring Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("land"), {QByteArrayLiteral("alice.txt")});
    addRow("Content Substring Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("derl"), {QByteArrayLiteral("alice.txt")});
    addRow("Content Case Insensitive Match",
           QByteArray("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("white"),
           {QByteArrayLiteral("Rabbit.TXT"), QByteArrayLiteral("rabbit.txt")});

    addRow("Content Word Apostrophe Match", QByteArray("syntax=regex&checkContent=yes"), QByteArrayLiteral("Alice's"), {QByteArrayLiteral("alice.txt")});
}

void FilenameSearchExternalTest::stringMatch()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/stringMatch");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QEXPECT_FAIL("Filename Match", "Should match filenames", Continue);

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchExternalTest::phraseMatch_data()
{
    addColumns();

    addRow("No Content Phrase Match", QByteArray("syntax=phrase&checkContent=yes"), QByteArrayLiteral("Cheshire Cat"), {});
    addRow("Content Exact Phrase Match",
           QByteArray("syntax=phrase&checkContent=yes"),
           QByteArrayLiteral("Adventures in Wonderland"),
           {QByteArrayLiteral("alice.txt")});

    //  Check explicit phrase syntax searches

    addRow("Content Phrase Query with Phrase Match",
           QByteArray("syntax=phrase&checkContent=yes"),
           QByteArrayLiteral("Adventures in Wonderland"),
           {QByteArrayLiteral("alice.txt")});
    addRow("No Content Phrase Query with Regex Match", QByteArray("syntax=phrase&checkContent=yes"), QByteArrayLiteral("Advent.* in Wond.*"), {});
}

void FilenameSearchExternalTest::phraseMatch()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/stringMatch");

    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchExternalTest::regexMatch_data()
{
    addColumns();

    addRow("Content Start of String Match",
           QByteArrayLiteral("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("^Walrus"),
           {QByteArrayLiteral("walrus.txt")});
    addRow("No Content Start of String Match", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("^Carpenter"), {});
    addRow("Content End of String Match",
           QByteArrayLiteral("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("Carpenter$"),
           {QByteArrayLiteral("walrus.txt")});
    addRow("No Content End of String Match", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("Walrus$"), {});

    addRow("Content Invalid Regular Expression", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("+"), {});
    addRow("Content Escaped Regex Special Character",
           QByteArrayLiteral("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("\\+"),
           {QByteArrayLiteral("walrus.txt")});
    addRow("No Content Escaped Regex Special Character", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("\\*"), {});
}

void FilenameSearchExternalTest::regexMatch()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/stringMatch");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QEXPECT_FAIL("Content Regex Query with Regex Match", "External Script cannot handle more than one term", Continue);
    QCOMPARE(results, expectedFiles);
}

void FilenameSearchExternalTest::wordListMatch_data()
{
    addColumns();

    //  Check searches for collections of words work rather than exact phrases.

    addRow("Content Soup Match", QByteArray("syntax=wordlist&checkContent=yes"), QByteArrayLiteral("Adventures Wonderland"), {QByteArrayLiteral("alice.txt")});
    addRow("Content Random Soup Match",
           QByteArray("syntax=wordlist&checkContent=yes"),
           QByteArrayLiteral("Wonderland Adventures"),
           {QByteArrayLiteral("alice.txt")});
}

void FilenameSearchExternalTest::wordListMatch()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/stringMatch");

    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QEXPECT_FAIL("Content Soup Match", "Should match collections of terms", Continue);
    QEXPECT_FAIL("Content Random Soup Match", "Should match disordered collections of terms", Continue);

    QCOMPARE(results, expectedFiles);
}

void FilenameSearchExternalTest::filenameContent_data()
{
    addColumns();

    addRow("No Content Match", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("wonderland"), {});
    addRow("Content Match", QByteArrayLiteral("syntax=regex&checkContent=yes"), QByteArrayLiteral("curiouser"), {QByteArrayLiteral("alice2.txt")});
}

void FilenameSearchExternalTest::filenameContent()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/filename-content");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QCOMPARE(results, expectedFiles);
}

//  Search including hidden files, to catch the .bak (that may be considered hidden)

//  A filename search for "alice" finds the .bak, backup file,
//  The external search script (assuming ripgrep) searches .bak files, whereas the simple search does not (they are not text/plain).

void FilenameSearchExternalTest::folderTree_data()
{
    addColumns();

    addRow("Content Match",
           QByteArray("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice1.txt.bak"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchExternalTest::folderTree()
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

void FilenameSearchExternalTest::folderTreeSymlinks_data()
{
    addColumns();

    addRow("Content Match",
           QByteArray("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice1.txt.bak"), QByteArrayLiteral("alice2.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchExternalTest::folderTreeSymlinks()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/folderTreeSymlinks/child");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), {});

    QEXPECT_FAIL("Content Match", "Should handle symlinked duplicates", Continue);
    QCOMPARE(results, expectedFiles);
}

void FilenameSearchExternalTest::hiddenFilesAndFolders_data()
{
    addColumns();

    addRow("Content Match",
           QByteArray("syntax=regex&checkContent=yes"),
           QByteArrayLiteral("Wonderland"),
           {QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});

    addRow("Content Match - Hidden Files and Folders",
           QByteArray("syntax=regex&includeHidden=yes&checkContent=yes"),
           QByteArrayLiteral("Wonderland"),
           {QByteArrayLiteral(".alice2.txt"), QByteArrayLiteral(".alice4.txt"), QByteArrayLiteral("alice1.txt"), QByteArrayLiteral("alice3.txt")});
}

void FilenameSearchExternalTest::hiddenFilesAndFolders()
{
    QFETCH(QByteArray, searchOptions);
    QFETCH(QByteArray, searchString);
    QFETCH(QByteArrayList, expectedFiles);

    const QString path = QFINDTESTDATA("data/hiddenFilesAndFolders");
    QByteArrayList results = doSearchQuery(buildSearchQuery(searchString, searchOptions, path), KIO::ListJob::ListFlag::IncludeHidden);

    QEXPECT_FAIL("Content Match", "Should ignore hidden files/folders by default", Continue);
    QCOMPARE(results, expectedFiles);
}

QTEST_MAIN(FilenameSearchExternalTest)

#include "filenamesearchexternaltest.moc"
