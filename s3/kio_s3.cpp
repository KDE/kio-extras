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
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>

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

S3Slave::S3Slave(const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("s3", pool_socket, app_socket)
{
    Q_UNUSED(protocol)

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    m_configProfileName = QByteArray::fromStdString(Aws::Auth::GetConfigProfileFilename());

    qCDebug(S3) << "kio_s3 ready.";
}

S3Slave::~S3Slave()
{
    qCDebug(S3) << "kio_s3 ended.";
}


void S3Slave::listDir(const QUrl &url)
{
    qCDebug(S3) << "Going to list" << url;
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Bucket:" << s3url.bucketName() << "Key:" << s3url.key();

    if (s3url.isRoot()) {
        listBuckets();
        listCwdEntry();
        finished();
        return;
    }

    if (s3url.isBucket()) {
        listBucket(s3url.bucketName());
        listCwdEntry();
        finished();
        return;
    }

    if (!s3url.isKey()) {
        qCDebug(S3) << "Could not list invalid S3 url:" << url;
        error(KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Invalid S3 URI, bucket name is missing from the host.<nl/>A valid S3 URI must be written in the form: <link>%1</link>", "s3://bucket/key"));
        return;
    }

    Q_ASSERT(s3url.isKey());

    listKey(s3url);
    listCwdEntry();
    finished();
}

void S3Slave::stat(const QUrl &url)
{
    qCDebug(S3) << "Going to stat()" << url;
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Bucket:" << s3url.bucketName() << "Key:" << s3url.key();

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

    if (!s3url.isKey()) {
        qCDebug(S3) << "Could not stat invalid S3 url:" << url;
        error(KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Invalid S3 URI, bucket name is missing from the host.<nl/>A valid S3 URI must be written in the form: <link>%1</link>", "s3://bucket/key"));
        return;
    }

    Q_ASSERT(s3url.isKey());

    // Try to do an HEAD request for the key.
    // If the URL is a folder, S3 will reply only if there is a 0-sized object with that key.
    const auto pathComponents = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    // The URL could be s3://<bucketName>/ which would have "/" as path(). Fallback to bucketName in that case.
    const auto fileName = pathComponents.isEmpty() ? s3url.bucketName() : pathComponents.last();

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::HeadObjectRequest headObjectRequest;
    headObjectRequest.SetBucket(s3url.bucketName().toStdString());
    headObjectRequest.SetKey(s3url.key().toStdString());

    auto headObjectRequestOutcome = client.HeadObject(headObjectRequest);
    if (headObjectRequestOutcome.IsSuccess()) {
        QString contentType = QString::fromStdString(headObjectRequestOutcome.GetResult().GetContentType());
        // This is set by S3 when creating a 0-sized folder from the AWS console. Use the freedesktop mimetype instead.
        if (contentType == QLatin1String("application/x-directory")) {
            contentType = QStringLiteral("inode/directory");
        }
        const bool isDir = contentType == QLatin1String("inode/directory");
        KIO::UDSEntry entry;
        entry.reserve(5);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, fileName);
        entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, fileName);
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, contentType);
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, isDir ? S_IFDIR : S_IFREG);
        entry.fastInsert(KIO::UDSEntry::UDS_SIZE, headObjectRequestOutcome.GetResult().GetContentLength());
        if (isDir) {
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
        } else {
            // For keys we would need another request (GetObjectAclRequest) to get the permission,
            // but it is kind of pointless to map the AWS ACL model to UNIX permission anyway.
            // So assume keys are always writable, we'll handle the failure if they are not.
            // The same logic will be applied to all the other UDS_ACCESS instances for keys.
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
        }

        statEntry(entry);
    } else {
        qCDebug(S3) << "Could not get HEAD object for key:" << s3url.key() << " - " << headObjectRequestOutcome.GetError().GetMessage().c_str();
        // Last chance: if the key ends with a slash, assume this is a folder (i.e. virtual key without associated object).
        if (s3url.key().endsWith(QLatin1Char('/'))) {
            KIO::UDSEntry entry;
            entry.reserve(6);
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, fileName);
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, fileName);
            entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
            statEntry(entry);
        }
    }

    finished();
}

void S3Slave::mimetype(const QUrl &url)
{
    qCDebug(S3) << "Going to get mimetype for" << url;
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Bucket:" << s3url.bucketName() << "Key:" << s3url.key();

    mimeType(contentType(s3url));
    finished();
}

void S3Slave::get(const QUrl &url)
{
    qCDebug(S3) << "Going to get" << url;
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Bucket:" << s3url.bucketName() << "Key:" << s3url.key();

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);

    const Aws::S3::S3Client client(clientConfiguration);

    mimeType(contentType(s3url));

    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.SetBucket(s3url.bucketName().toStdString());
    objectRequest.SetKey(s3url.key().toStdString());

    auto getObjectOutcome = client.GetObject(objectRequest);
    if (getObjectOutcome.IsSuccess()) {
        auto objectResult = getObjectOutcome.GetResultWithOwnership();
        auto& retrievedFile = objectResult.GetBody();
        qCDebug(S3) << "Key" << s3url.key() << "has Content-Length:" << objectResult.GetContentLength();
        totalSize(objectResult.GetContentLength());

        std::array<char, 1024*1024> buffer{};
        while (!retrievedFile.eof()) {
            const auto readBytes = retrievedFile.read(buffer.data(), buffer.size()).gcount();
            if (readBytes > 0) {
                data(QByteArray(buffer.data(), readBytes));
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
    Q_UNUSED(flags)
    qCDebug(S3) << "Going to upload data to" << url;
    const auto s3url = S3Url(url);

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(s3url.bucketName().toStdString());
    request.SetKey(s3url.key().toStdString());

    const auto putDataStream = std::make_shared<Aws::StringStream>("");

    int bytesCount = 0;
    int n;
    do {
        QByteArray buffer;
        dataReq();
        n = readData(buffer);
        bytesCount += n;
        if (!buffer.isEmpty()) {
            putDataStream->write(buffer.data(), n);
        }
    } while (n > 0);

    if (bytesCount <= 0) {
        error(KIO::ERR_CANNOT_WRITE, url.toDisplayString());
        return;
    }

    request.SetBody(putDataStream);

    auto putObjectOutcome = client.PutObject(request);
    if (putObjectOutcome.IsSuccess()) {
        qCDebug(S3) << "Uploaded" <<  bytesCount << "bytes to key:" << s3url.key();
    } else {
        qCDebug(S3) << "Could not PUT object with key:" << s3url.key() << " - " << putObjectOutcome.GetError().GetMessage().c_str();
    }

    finished();
}

void S3Slave::copy(const QUrl &src, const QUrl &dest, int, KIO::JobFlags flags)
{
    Q_UNUSED(flags)
    qCDebug(S3) << "Going to copy" << src << "to" << dest;

    const auto s3src = S3Url(src);
    const auto s3dest = S3Url(dest);

    if (s3src.isRoot() || s3src.isBucket()) {
        qCDebug(S3) << "Cannot copy from root or bucket url:" << src;
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, src.toDisplayString());
        return;
    }

    if (!s3src.isKey()) {
        qCDebug(S3) << "Cannot copu from invalid S3 url:" << src;
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, src.toDisplayString());
        return;
    }

    // TODO: can we copy to isBucket() urls?
    if (s3dest.isRoot() || s3dest.isBucket()) {
        qCDebug(S3) << "Cannot copy to root or bucket url:" << dest;
        error(KIO::ERR_WRITE_ACCESS_DENIED, dest.toDisplayString());
        return;
    }

    if (!s3dest.isKey()) {
        qCDebug(S3) << "Cannot write to invalid S3 url:" << dest;
        error(KIO::ERR_WRITE_ACCESS_DENIED, dest.toDisplayString());
        return;
    }

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::CopyObjectRequest request;
    request.SetCopySource(QStringLiteral("%1/%2").arg(s3src.bucketName(), s3src.key()).toStdString());
    request.SetBucket(s3dest.bucketName().toStdString());
    request.SetKey(s3dest.key().toStdString());

    auto copyObjectOutcome = client.CopyObject(request);
    if (!copyObjectOutcome.IsSuccess()) {
        qCDebug(S3) << "Could not copy" << src << "to" << dest << "- " << copyObjectOutcome.GetError().GetMessage().c_str();
        error(KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Could not copy <link>%1</link> to <link>%2</link>", src.toDisplayString(), dest.toDisplayString()));
        return;
    }

    finished();
}

void S3Slave::mkdir(const QUrl &url, int)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::del(const QUrl &url, bool)
{
    qCDebug(S3) << "Going to delete" << url;
    const auto s3url = S3Url(url);

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(s3url.bucketName().toStdString());
    // FIXME: what about folders? how should we handle them?
    request.SetKey(s3url.key().toStdString());

    auto deleteObjectOutcome = client.DeleteObject(request);
    if (!deleteObjectOutcome.IsSuccess()) {
        qCDebug(S3) << "Could not delete object with key:" << s3url.key() << " - " << deleteObjectOutcome.GetError().GetMessage().c_str();
        error(KIO::ERR_CANNOT_DELETE, url.toDisplayString());
    } else {
        finished();
    }
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
    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);
    const auto listBucketsOutcome = client.ListBuckets();

    if (listBucketsOutcome.IsSuccess()) {
        const auto buckets = listBucketsOutcome.GetResult().GetBuckets();
        for (const auto &bucket : buckets) {
            const auto bucketName = QString::fromStdString(bucket.GetName());
            qCDebug(S3) << "Found bucket:" << bucketName;
            KIO::UDSEntry entry;
            entry.reserve(7);
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, bucketName);
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, bucketName);
            entry.fastInsert(KIO::UDSEntry::UDS_URL, QStringLiteral("s3://%1/").arg(bucketName));
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
            entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("folder-network"));
            listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list buckets:" << listBucketsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Slave::listBucket(const QString &bucketName)
{
    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
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
            entry.reserve(6);
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, object.GetKey().c_str());
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, object.GetKey().c_str());
            entry.fastInsert(KIO::UDSEntry::UDS_URL, QStringLiteral("s3://%1/%2").arg(bucketName, object.GetKey().c_str()));
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, object.GetSize());
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
            listEntry(entry);
        }

        const auto commonPrefixes = listObjectsOutcome.GetResult().GetCommonPrefixes();
        for (const auto &commonPrefix : commonPrefixes) {
            KIO::UDSEntry entry;
            QString prefix = QString::fromStdString(commonPrefix.GetPrefix());
            if (prefix.endsWith(QLatin1Char('/'))) {
                prefix.chop(1);
            }
            entry.reserve(6);
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, prefix);
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, prefix);
            entry.fastInsert(KIO::UDSEntry::UDS_URL, QStringLiteral("s3://%1/%2/").arg(bucketName, prefix));
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
            entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);

            listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list bucket: " << listObjectsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Slave::listKey(const S3Url &s3url)
{
    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
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
            QString key = QString::fromStdString(object.GetKey());
            // Note: key might be empty. 0-sized virtual folders have object.GetKey() equal to prefix.
            key.remove(0, prefix.length());

            KIO::UDSEntry entry;
            // S3 always appends trailing slash to "folder" objects.
            if (key.endsWith(QLatin1Char('/'))) {
                entry.reserve(5);
                entry.fastInsert(KIO::UDSEntry::UDS_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
                entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
                entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
                listEntry(entry);
            } else if (!key.isEmpty()) { // Not a folder.
                entry.reserve(5);
                entry.fastInsert(KIO::UDSEntry::UDS_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
                entry.fastInsert(KIO::UDSEntry::UDS_SIZE, object.GetSize());
                entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
                listEntry(entry);
            }
        }

        const auto commonPrefixes = listObjectsOutcome.GetResult().GetCommonPrefixes();
        qCDebug(S3) << "Prefix" << prefix << "has" << commonPrefixes.size() << "common prefixes";
        for (const auto &commonPrefix : commonPrefixes) {
            QString subprefix = QString::fromStdString(commonPrefix.GetPrefix());
            if (subprefix.endsWith(QLatin1Char('/'))) {
                subprefix.chop(1);
            }
            if (subprefix.startsWith(prefix)) {
                subprefix.remove(0, prefix.length());
            }
            KIO::UDSEntry entry;
            entry.reserve(4);
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

void S3Slave::listCwdEntry()
{
    // List a non-writable UDSEntry for "."
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);
}

QString S3Slave::contentType(const S3Url &s3url)
{
    QString contentType;

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::HeadObjectRequest headObjectRequest;
    headObjectRequest.SetBucket(s3url.bucketName().toStdString());
    headObjectRequest.SetKey(s3url.key().toStdString());

    auto headObjectRequestOutcome = client.HeadObject(headObjectRequest);
    if (headObjectRequestOutcome.IsSuccess()) {
        contentType = QString::fromStdString(headObjectRequestOutcome.GetResult().GetContentType());
        qCDebug(S3) << "Key" << s3url.key() << "has Content-Type:" << contentType;
    } else {
        qCDebug(S3) << "Could not get content type for key:" << s3url.key() << " - " << headObjectRequestOutcome.GetError().GetMessage().c_str();
    }

    return contentType;
}

#include "kio_s3.moc"
