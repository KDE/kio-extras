/*
 *  Copyright (C) 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "kio_s3.h"
#include "s3debug.h"
#include "s3url.h"

#include <KLocalizedString>

#include <QCoreApplication>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>

class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.s3" FILE "s3.json")
};

extern "C"
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_s3"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_s3 protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    S3Slave slave(argv[1], argv[2], argv[3]);
    qCDebug(S3) << "Starting kio_s3...";
    slave.dispatchLoop();

    return 0;
}

S3Slave::S3Slave(const QByteArray &protocol, const QByteArray &pool_socket,
                      const QByteArray &app_socket):
    SlaveBase("s3", pool_socket, app_socket)
{
    Q_UNUSED(protocol)

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    qCDebug(S3) << "kio_s3 ready";
}

S3Slave::~S3Slave()
{
    qCDebug(S3) << "kio_s3 ready";
}


void S3Slave::listDir(const QUrl &url)
{
    qCDebug(S3) << "Going to list" << url;

    QString prefix = url.path().remove(0, 1);
    if (!prefix.endsWith(QLatin1Char('/'))) {
        prefix += QLatin1Char('/');
    }


    const auto s3url = S3Url(url);
    qCDebug(S3) << "bucketName:" << s3url.bucketName() << "key" << s3url.key();

    if (s3url.isRoot()) {
        listBuckets();
        finished();
        return;
    }

//    if (!m_bucketNames.contains(s3url.bucketName())) {
//        qCDebug(S3) << "Unknown bucket" << s3url.bucketName() << "for" << url;
//        error(KIO::ERR_SLAVE_DEFINED, i18n("%1 is not a known S3 account", s3url.bucketName()));
//        return;
//    }

    if (s3url.isBucket()) {
        listBucket(s3url.bucketName());
        finished();
        return;
    }

    listFolder(s3url);

    // We also need a non-null and writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);

    finished();
}

void S3Slave::stat(const QUrl &url)
{
    qCDebug(S3) << "Going to stat()" << url;
    const auto s3url = S3Url(url);
    qCDebug(S3) << "bucketName:" << s3url.bucketName() << "key" << s3url.key();

    if (s3url.isRoot()) {
        finished();
        return;
    }

    if (s3url.isBucket()) {
        KIO::UDSEntry entry;
        entry.reserve(4);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, s3url.bucketName());
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
        statEntry(entry);
        finished();
        return;
    }

//    Q_ASSERT(s3url.isKey());

    finished();
}

void S3Slave::mimetype(const QUrl &url)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::get(const QUrl &url)
{
    qCDebug(S3) << "Going to get" << url;
    const auto s3url = S3Url(url);

    const auto configProfileName = Aws::Auth::GetConfigProfileName();   // This is needed to make the SDK get the proper region from ~/.aws/config
    const Aws::Client::ClientConfiguration clientConfiguration(configProfileName.c_str());

    const Aws::S3::S3Client client(clientConfiguration);
    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.SetBucket(s3url.bucketName().toStdString());
    objectRequest.SetKey(s3url.key().toStdString());

    auto getObjectOutcome = client.GetObject(objectRequest);
    if (getObjectOutcome.IsSuccess()) {
        auto& retrievedFile = getObjectOutcome.GetResultWithOwnership().GetBody();

        QByteArray contentData;
        char buffer[1024 * 1024] = { 0 };
        while (!retrievedFile.eof()) {
            const auto readBytes = retrievedFile.read(buffer, sizeof(buffer)).gcount();
            if (readBytes > 0) {
                data(QByteArray(buffer, readBytes));
            }
        }

        data(QByteArray());

    } else {
        qCDebug(S3) << "Could not get object with key:" << s3url.key() << " - " << getObjectOutcome.GetError().GetMessage().c_str();
    }

    finished();
}

void S3Slave::put(const QUrl &url, int, KIO::JobFlags flags)
{
    Q_UNUSED(url)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::copy(const QUrl &src, const QUrl &dest, int, KIO::JobFlags flags)
{
    Q_UNUSED(src)
    Q_UNUSED(dest)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::mkdir(const QUrl &url, int)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::del(const QUrl &url, bool)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    Q_UNUSED(src)
    Q_UNUSED(dest)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::listBuckets()
{
    if (m_bucketNamesCache.isEmpty()) {

        const auto configProfileName = Aws::Auth::GetConfigProfileName();   // This is needed to make the SDK get the proper region from ~/.aws/config
        const Aws::Client::ClientConfiguration clientConfiguration(configProfileName.c_str());

        const Aws::S3::S3Client client(clientConfiguration);
        const auto listBucketsOutcome = client.ListBuckets();

        if (listBucketsOutcome.IsSuccess()) {
            const auto buckets = listBucketsOutcome.GetResult().GetBuckets();
            for (const auto &bucket : buckets) {
                const auto bucketName = QString::fromStdString(bucket.GetName());
                qCDebug(S3) << "Found bucket:" << bucketName;
                m_bucketNamesCache << bucketName;

            }
        } else {
            qCDebug(S3) << "Could not list buckets:" << listBucketsOutcome.GetError().GetMessage().c_str();
        }
    }

    for (const auto &bucketName : m_bucketNamesCache) {
        KIO::UDSEntry entry;
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, bucketName);
        entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, bucketName);
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("folder-network"));
        listEntry(entry);
    }

    // Create also non-writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);
}

void S3Slave::listBucket(const QString &bucketName)
{
    const auto configProfileName = Aws::Auth::GetConfigProfileName();   // This is needed to make the SDK get the proper region from ~/.aws/config
    const Aws::Client::ClientConfiguration clientConfiguration(configProfileName.c_str());

    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
    listObjectsRequest.SetBucket(bucketName.toStdString());
    listObjectsRequest.SetDelimiter("/");

    qCDebug(S3) << "Listing objects in bucket" << bucketName << "...";
    const auto listObjectsOutcome = client.ListObjectsV2(listObjectsRequest);

    if (listObjectsOutcome.IsSuccess()) {

        const auto objects = listObjectsOutcome.GetResult().GetContents();
        for (const auto &object : objects) {
            KIO::UDSEntry entry;
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, object.GetKey().c_str());
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, object.GetKey().c_str());
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, object.GetSize());
            listEntry(entry);
        }

        const auto commonPrefixes = listObjectsOutcome.GetResult().GetCommonPrefixes();
        for (const auto &commonPrefix : commonPrefixes) {
            KIO::UDSEntry entry;
            QString prefix = QString::fromStdString(commonPrefix.GetPrefix());
            if (prefix.endsWith(QLatin1Char('/'))) {
                prefix.chop(1);
            }

            entry.fastInsert(KIO::UDSEntry::UDS_NAME, prefix);
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, prefix);
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);

            listEntry(entry);
        }

    } else {
        qCDebug(S3) << "Could not list bucket: " << listObjectsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Slave::listFolder(const S3Url &s3url)
{
    const auto configProfileName = Aws::Auth::GetConfigProfileName();   // This is needed to make the SDK get the proper region from ~/.aws/config
    const Aws::Client::ClientConfiguration clientConfiguration(configProfileName.c_str());

    const Aws::S3::S3Client client(clientConfiguration);

    const QString prefix = s3url.prefix();

    Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
    listObjectsRequest.SetBucket(s3url.bucketName().toStdString());
    listObjectsRequest.SetDelimiter("/");
    listObjectsRequest.SetPrefix(prefix.toStdString());

    qCDebug(S3) << "Listing prefix" << prefix << "...";
    const auto listObjectsOutcome = client.ListObjectsV2(listObjectsRequest);

    if (listObjectsOutcome.IsSuccess()) {

        const auto objects = listObjectsOutcome.GetResult().GetContents();
        // TODO: handle listObjectsOutcome.GetResult().GetIsTruncated()
        // By default the max-keys request parameter is 1000, which is reasonable for us
        // since we filter the keys by the name of the folder, but it won't work
        // if someone has very big folders with more than 1000 files.
        qCDebug(S3) << "Prefix" << prefix << "has" << objects.size() << "objects";
        for (const auto &object : objects) {
            qCDebug(S3) << "Found object:" << object.GetKey().c_str();

            QString key = QString::fromStdString(object.GetKey());
            key.remove(0, prefix.length());
            qCDebug(S3) << "Going to list key:" << key;

            KIO::UDSEntry entry;
            // S3 always appends trailing slash to "folder" objects.
            if (key.endsWith(QLatin1Char('/'))) {
                entry.fastInsert(KIO::UDSEntry::UDS_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
                entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
                listEntry(entry);
            } else { // Not a folder.
                entry.fastInsert(KIO::UDSEntry::UDS_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
                entry.fastInsert(KIO::UDSEntry::UDS_SIZE, object.GetSize());
                listEntry(entry);

            }

        }

        const auto commonPrefixes = listObjectsOutcome.GetResult().GetCommonPrefixes();
        qCDebug(S3) << "Prefix" << s3url.key() << "has" << commonPrefixes.size() << "prefixes";
        for (const auto &commonPrefix : commonPrefixes) {
            qCDebug(S3) << "Found commonPrefix:" << commonPrefix.GetPrefix().c_str();

            QString subprefix = QString::fromStdString(commonPrefix.GetPrefix());
            if (subprefix.endsWith(QLatin1Char('/'))) {
                subprefix.chop(1);
            }
            if (subprefix.startsWith(prefix)) {
                subprefix.remove(0, prefix.length());
            }
            KIO::UDSEntry entry;
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, subprefix);
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, subprefix);
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);

            listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list prefix" << s3url.key() << " - " << listObjectsOutcome.GetError().GetMessage().c_str();
    }
}

#include "kio_s3.moc"
