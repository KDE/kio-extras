/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "s3backend.h"
#include "s3debug.h"
#include "kio_s3.h"

#include <KLocalizedString>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/Aws.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>

static S3Backend::Result finished() {
    static const S3Backend::Result s_finished = {0, QString()};
    return s_finished; // frontend should emit finished().
}

S3Backend::S3Backend(S3Slave *q)
    : q(q)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    m_configProfileName = QByteArray::fromStdString(Aws::Auth::GetConfigProfileFilename());
}

S3Backend::Result S3Backend::listDir(const QUrl &url)
{
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to list" << s3url;

    if (s3url.isRoot()) {
        listBuckets();
        listCwdEntry(ReadOnlyCwd);
        return finished();
    }

    if (s3url.isBucket()) {
        listBucket(s3url.bucketName());
        listCwdEntry();
        return finished();
    }

    if (!s3url.isKey()) {
        qCDebug(S3) << "Could not list invalid S3 url:" << url;
        return {KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Invalid S3 URI, bucket name is missing from the host.<nl/>A valid S3 URI must be written in the form: <link>%1</link>", "s3://bucket/key")};
    }

    Q_ASSERT(s3url.isKey());

    listKey(s3url);
    listCwdEntry();
    return finished();
}

S3Backend::Result S3Backend::stat(const QUrl &url)
{
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to stat()" << s3url;

    if (s3url.isRoot()) {
        return finished();
    }

    if (s3url.isBucket()) {
        KIO::UDSEntry entry;
        entry.reserve(4);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, s3url.bucketName());
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
        q->statEntry(entry);
        return finished();
    }

    if (!s3url.isKey()) {
        qCDebug(S3) << "Could not stat invalid S3 url:" << url;
        return {KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Invalid S3 URI, bucket name is missing from the host.<nl/>A valid S3 URI must be written in the form: <link>%1</link>", "s3://bucket/key")};
    }

    Q_ASSERT(s3url.isKey());

    // Try to do an HEAD request for the key.
    // If the URL is a folder, S3 will reply only if there is a 0-sized object with that key.
    const auto pathComponents = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    // The URL could be s3://<bucketName>/ which would have "/" as path(). Fallback to bucketName in that case.
    const bool isRootKey = pathComponents.isEmpty();
    const auto fileName = isRootKey ? s3url.bucketName() : pathComponents.last();

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    Aws::S3::Model::HeadObjectRequest headObjectRequest;
    headObjectRequest.SetBucket(s3url.bucketName().toStdString());
    headObjectRequest.SetKey(s3url.key().toStdString());

    auto headObjectRequestOutcome = client.HeadObject(headObjectRequest);
    if (!isRootKey && headObjectRequestOutcome.IsSuccess()) {
        QString contentType = QString::fromStdString(headObjectRequestOutcome.GetResult().GetContentType());
        // This is set by S3 when creating a 0-sized folder from the AWS console. Use the freedesktop mimetype instead.
        if (contentType == QLatin1String("application/x-directory")) {
            contentType = QStringLiteral("inode/directory");
        }
        const bool isDir = contentType == QLatin1String("inode/directory");
        KIO::UDSEntry entry;
        entry.reserve(7);
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
            const auto lastModifiedTime = headObjectRequestOutcome.GetResult().GetLastModified();
            entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, lastModifiedTime.SecondsWithMSPrecision());
        }

        q->statEntry(entry);
    } else {
        if (!isRootKey) {
            qCDebug(S3).nospace() << "Could not get HEAD object for key: " << s3url.key() << " - " << headObjectRequestOutcome.GetError().GetMessage().c_str() << " - assuming it's a folder.";
        }
        // HACK: assume this is a folder (i.e. a virtual key without associated object).
        // If it were a key or a 0-sized folder the HEAD request would likely have worked.
        // This is needed to upload local folders to S3.
        KIO::UDSEntry entry;
        entry.reserve(6);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, fileName);
        entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, fileName);
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
        q->statEntry(entry);
    }

    return finished();
}

S3Backend::Result S3Backend::mimetype(const QUrl &url)
{
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to get mimetype for" << s3url;

    q->mimeType(contentType(s3url));
    return finished();
}

S3Backend::Result S3Backend::get(const QUrl &url)
{
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to get" << s3url;

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    q->mimeType(contentType(s3url));

    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.SetBucket(s3url.bucketName().toStdString());
    objectRequest.SetKey(s3url.key().toStdString());

    auto getObjectOutcome = client.GetObject(objectRequest);
    if (getObjectOutcome.IsSuccess()) {
        auto objectResult = getObjectOutcome.GetResultWithOwnership();
        auto& retrievedFile = objectResult.GetBody();
        qCDebug(S3) << "Key" << s3url.key() << "has Content-Length:" << objectResult.GetContentLength();
        q->totalSize(objectResult.GetContentLength());

        std::array<char, 1024*1024> buffer{};
        while (!retrievedFile.eof()) {
            const auto readBytes = retrievedFile.read(buffer.data(), buffer.size()).gcount();
            if (readBytes > 0) {
                q->data(QByteArray(buffer.data(), readBytes));
            }
        }

        q->data(QByteArray());

    } else {
        qCDebug(S3) << "Could not get object with key:" << s3url.key() << " - " << getObjectOutcome.GetError().GetMessage().c_str();
    }

    return finished();
}

S3Backend::Result S3Backend::put(const QUrl &url, int permissions, KIO::JobFlags flags)
{
    Q_UNUSED(permissions)
    Q_UNUSED(flags)
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to upload data to" << s3url;

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
        q->dataReq();
        n = q->readData(buffer);
        if (!buffer.isEmpty()) {
            bytesCount += n;
            putDataStream->write(buffer.data(), n);
        }
    } while (n > 0);

    if (n <= 0) {
        return {KIO::ERR_CANNOT_WRITE, url.toDisplayString()};
    }

    request.SetBody(putDataStream);

    auto putObjectOutcome = client.PutObject(request);
    if (putObjectOutcome.IsSuccess()) {
        qCDebug(S3) << "Uploaded" <<  bytesCount << "bytes to key:" << s3url.key();
    } else {
        qCDebug(S3) << "Could not PUT object with key:" << s3url.key() << " - " << putObjectOutcome.GetError().GetMessage().c_str();
    }

    return finished();
}

S3Backend::Result S3Backend::copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags)
{
    Q_UNUSED(permissions)
    Q_UNUSED(flags)

    const auto s3src = S3Url(src);
    const auto s3dest = S3Url(dest);
    qCDebug(S3) << "Going to copy" << s3src << "to" << s3dest;

    if (src == dest) {
        return {KIO::ERR_FILE_ALREADY_EXIST, QString()};
    }

    if (s3src.isRoot() || s3src.isBucket()) {
        qCDebug(S3) << "Cannot copy from root or bucket url:" << src;
        return {KIO::ERR_CANNOT_OPEN_FOR_READING, src.toDisplayString()};
    }

    if (!s3src.isKey()) {
        qCDebug(S3) << "Cannot copy from invalid S3 url:" << src;
        return {KIO::ERR_CANNOT_OPEN_FOR_READING, src.toDisplayString()};
    }

    // TODO: can we copy to isBucket() urls?
    if (s3dest.isRoot() || s3dest.isBucket()) {
        qCDebug(S3) << "Cannot copy to root or bucket url:" << dest;
        return {KIO::ERR_WRITE_ACCESS_DENIED, dest.toDisplayString()};
    }

    if (!s3dest.isKey()) {
        qCDebug(S3) << "Cannot write to invalid S3 url:" << dest;
        return {KIO::ERR_WRITE_ACCESS_DENIED, dest.toDisplayString()};
    }

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    // Check if destination key already exists, otherwise S3 will overwrite it leading to data loss.
    Aws::S3::Model::HeadObjectRequest headObjectRequest;
    headObjectRequest.SetBucket(s3dest.bucketName().toStdString());
    headObjectRequest.SetKey(s3dest.key().toStdString());
    auto headObjectRequestOutcome = client.HeadObject(headObjectRequest);
    if (headObjectRequestOutcome.IsSuccess()) {
        return {KIO::ERR_FILE_ALREADY_EXIST, QString()};
    }

    Aws::S3::Model::CopyObjectRequest request;
    request.SetCopySource(QStringLiteral("%1/%2").arg(s3src.bucketName(), s3src.key()).toStdString());
    request.SetBucket(s3dest.bucketName().toStdString());
    request.SetKey(s3dest.key().toStdString());

    auto copyObjectOutcome = client.CopyObject(request);
    if (!copyObjectOutcome.IsSuccess()) {
        qCDebug(S3) << "Could not copy" << src << "to" << dest << "- " << copyObjectOutcome.GetError().GetMessage().c_str();
        return {KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Could not copy <link>%1</link> to <link>%2</link>", src.toDisplayString(), dest.toDisplayString())};
    }

    return finished();
}

S3Backend::Result S3Backend::mkdir(const QUrl &url, int permissions)
{
    Q_UNUSED(url)
    Q_UNUSED(permissions)
    qCDebug(S3) << "Pretending creation of folder" << url;
    return finished();
}

S3Backend::Result S3Backend::del(const QUrl &url, bool isFile)
{
    Q_UNUSED(isFile)
    const auto s3url = S3Url(url);
    qCDebug(S3) << "Going to delete" << s3url;

    if (s3url.isRoot() || s3url.isBucket()) {
        return {KIO::ERR_CANNOT_DELETE, url.toDisplayString()};
    }

    if (!s3url.isKey()) {
        return {KIO::ERR_SLAVE_DEFINED, xi18nc("@info", "Invalid S3 URI, bucket name is missing from the host.<nl/>A valid S3 URI must be written in the form: <link>%1</link>", "s3://bucket/key")};
    }

    const Aws::Client::ClientConfiguration clientConfiguration(m_configProfileName);
    const Aws::S3::S3Client client(clientConfiguration);

    // Start recursive delete by using the root prefix.
    if (deletePrefix(client, s3url, s3url.prefix())) {
        return finished();
    } else {
        return {KIO::ERR_CANNOT_DELETE, url.toDisplayString()};
    }
}

S3Backend::Result S3Backend::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    Q_UNUSED(flags)
    qCDebug(S3) << "Going to rename" << src << "to" << dest;

    // FIXME: rename of virtual folders doesn't work, because folders don't exist in S3.
    // This would require some special handling:
    // 1. detect that src is a folder
    // 2. list the folder
    // 3. rename each key listed
    // Workaround: copy+delete from dolphin...

    const auto copyResult = copy(src, dest, -1, flags);
    if (copyResult.exitCode > 0) {
        qCDebug(S3).nospace() << "Could not copy " << src << " to " << dest << ", aborting rename()";
        if (copyResult.exitCode == KIO::ERR_FILE_ALREADY_EXIST) {
            return {KIO::ERR_FILE_ALREADY_EXIST, QString()};
        }
        return {KIO::ERR_CANNOT_RENAME, src.toDisplayString()};
    }

    const auto delResult = del(src, false);
    if (delResult.exitCode > 0) {
        qCDebug(S3) << "Could not delete" << src << "after it was copied to" << dest;
        return {KIO::ERR_CANNOT_RENAME, src.toDisplayString()};
    }

    return finished();
}

void S3Backend::listBuckets()
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
            q->listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list buckets:" << listBucketsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Backend::listBucket(const QString &bucketName)
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
            q->listEntry(entry);
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

            q->listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list bucket: " << listObjectsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Backend::listKey(const S3Url &s3url)
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
                q->listEntry(entry);
            } else if (!key.isEmpty()) { // Not a folder.
                entry.reserve(6);
                entry.fastInsert(KIO::UDSEntry::UDS_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, key);
                entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
                entry.fastInsert(KIO::UDSEntry::UDS_SIZE, object.GetSize());
                entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
                entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, object.GetLastModified().SecondsWithMSPrecision());
                q->listEntry(entry);
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

            q->listEntry(entry);
        }
    } else {
        qCDebug(S3) << "Could not list prefix" << s3url.key() << " - " << listObjectsOutcome.GetError().GetMessage().c_str();
    }
}

void S3Backend::listCwdEntry(CwdAccess access)
{
    // List UDSEntry for "."
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    if (access == ReadOnlyCwd) {
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    } else {
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

    q->listEntry(entry);
}

bool S3Backend::deletePrefix(const Aws::S3::S3Client &client, const S3Url &s3url, const QString &prefix)
{
    qCDebug(S3) << "Going to recursively delete prefix:" << prefix;
    bool outcome = false;
    const auto bucketName = s3url.bucketName().toStdString();
    // In order to recursively delete a folder, we need to list by prefix and manually delete each listed key.
    Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
    listObjectsRequest.SetBucket(bucketName);
    listObjectsRequest.SetDelimiter("/");
    listObjectsRequest.SetPrefix(prefix.toStdString());
    const auto listObjectsOutcome = client.ListObjectsV2(listObjectsRequest);
    if (listObjectsOutcome.IsSuccess()) {
        // TODO: handle listObjectsOutcome.GetResult().GetIsTruncated()
        // By default the max-keys request parameter is 1000, which is reasonable for us
        // since we filter the keys by the name of the folder, but it won't work
        // if someone has very big folders with more than 1000 files.
        const auto commonPrefixes = listObjectsOutcome.GetResult().GetCommonPrefixes();
        qCDebug(S3) << "Prefix" << prefix << "has" << commonPrefixes.size() << "common prefixes";
        // Recursively delete folder children.
        for (const auto &commonPrefix : commonPrefixes) {
            const bool recursiveOutcome = deletePrefix(client, s3url, QString::fromStdString(commonPrefix.GetPrefix()));
            outcome = outcome && recursiveOutcome;
        }
        const auto objects = listObjectsOutcome.GetResult().GetContents();
        // Delete each file child.
        if (objects.size() > 0) {
            for (const auto &object : objects) {
                Aws::S3::Model::DeleteObjectRequest request;
                request.SetBucket(bucketName);
                request.SetKey(object.GetKey());
                auto deleteObjectOutcome = client.DeleteObject(request);
                if (!deleteObjectOutcome.IsSuccess()) {
                    qCDebug(S3) << "Could not delete object with key:" << s3url.key() << " - " << deleteObjectOutcome.GetError().GetMessage().c_str();
                    outcome = false;
                }
            }
        } else { // The prefix was either a file or a folder that contains 0 files.
            Aws::S3::Model::DeleteObjectRequest request;
            request.SetBucket(bucketName);
            request.SetKey(s3url.key().toStdString());
            auto deleteObjectOutcome = client.DeleteObject(request);
            if (!deleteObjectOutcome.IsSuccess()) {
                qCDebug(S3) << "Could not delete object with key:" << s3url.key() << " - " << deleteObjectOutcome.GetError().GetMessage().c_str();
                outcome = false;
            }
        }
        outcome = true;
    } else {
        qCDebug(S3) << "Could not list prefix:" << prefix;
        outcome = false;
    }

    return outcome;
}

QString S3Backend::contentType(const S3Url &s3url)
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
