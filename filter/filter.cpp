/*
This file is part of KDE

 SPDX-FileCopyrightText: 1999-2000 Waldo Bastian <bastian@kde.org>
 SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

SPDX-License-Identifier: MIT
*/

#include "filter.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QMimeType>
#include <QMimeDatabase>

#include <KFilterBase>
#include <KCompressionDevice>

#include "loggingcategory.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.filter" FILE "filter.json")
};

extern "C" {
    Q_DECL_EXPORT int kdemain(int argc, char **argv);
}

int kdemain( int argc, char ** argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_filter");

    qDebug(KIO_FILTER_DEBUG) << "Starting";

    if (argc != 4)
    {
        fprintf(stderr, "Usage: kio_filter protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    FilterProtocol worker(argv[1], argv[2], argv[3]);
    worker.dispatchLoop();

    qDebug(KIO_FILTER_DEBUG) << "Done";
    return 0;
}

FilterProtocol::FilterProtocol( const QByteArray & protocol, const QByteArray &pool, const QByteArray &app )
    : KIO::WorkerBase( protocol, pool, app )
    , m_protocol(QString::fromLatin1(protocol))
{
    const QString mimetype =
        (protocol == "zstd") ? QStringLiteral("application/zstd") :
        QLatin1String("application/x-") + QLatin1String(protocol.constData());

    filter = KCompressionDevice::filterForCompressionType(KCompressionDevice::compressionTypeForMimeType( mimetype ));
    Q_ASSERT(filter);
}

KIO::WorkerResult FilterProtocol::get(const QUrl& url)
{
    // In the old solution, subURL would be set by setSubURL.
    // KDE4: now I simply assume bzip2:/localpath/file.bz2 and set subURL to the local path.
    QUrl subURL = url;
    subURL.setScheme("file");

    if (subURL.isEmpty()) {
        return KIO::WorkerResult::fail(KIO::ERR_NO_SOURCE_PROTOCOL, m_protocol);
    }

    QFile localFile(url.path());
    if (!localFile.open(QIODevice::ReadOnly)) {
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_READ, m_protocol);
    }

    if (!filter) {
        // TODO better error message
        return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, m_protocol);
    }

    filter->init(QIODevice::ReadOnly);

    bool bNeedHeader = true;
    bool bNeedMimetype = true;
    bool bError = true;
    int result;

    QByteArray inputBuffer;
    inputBuffer.resize(8*1024);
    QByteArray outputBuffer;
    outputBuffer.resize(8*1024); // Start with a modest buffer
    filter->setOutBuffer( outputBuffer.data(), outputBuffer.size() );
    while(true)
    {
        if (filter->inBufferEmpty())
        {
            result = localFile.read(inputBuffer.data(), inputBuffer.size());
            qDebug(KIO_FILTER_DEBUG) << "requestData: got " << result;
            if (result <= 0)
            {
                bError = true;
                break; // Unexpected EOF.
            }
            filter->setInBuffer( inputBuffer.data(), inputBuffer.size() );
        }
        if (bNeedHeader)
        {
            bError = !filter->readHeader();
            if (bError)
                break;
            bNeedHeader = false;
        }
        result = filter->uncompress();
        if ((filter->outBufferAvailable() == 0) || (result == KFilterBase::End))
        {
            qDebug(KIO_FILTER_DEBUG) << "avail_out = " << filter->outBufferAvailable();
            if (filter->outBufferAvailable() != 0)
            {
                // Discard unused space :-)
                outputBuffer.resize(outputBuffer.size() - filter->outBufferAvailable());
            }
            if (bNeedMimetype) {
                // Can we use the "base" filename? E.g. foo.txt.bz2
                const QString extension = QFileInfo(subURL.path()).suffix();
                QMimeDatabase db;
                QMimeType mime;
                if (extension == "gz" || extension == "bz" || extension == "bz2"|| extension == "zst") {
                    QString baseName = subURL.path();
                    baseName.truncate(baseName.length() - extension.length() - 1 /*the dot*/);
                    qDebug(KIO_FILTER_DEBUG) << "baseName=" << baseName;
                    mime = db.mimeTypeForFileNameAndData(baseName, outputBuffer);
                } else {
                    mime = db.mimeTypeForData(outputBuffer);
                }
                qDebug(KIO_FILTER_DEBUG) << "Emitting mimetype " << mime.name();
                mimeType( mime.name() );
                bNeedMimetype = false;
            }
            data( outputBuffer ); // Send data
            filter->setOutBuffer( outputBuffer.data(), outputBuffer.size() );
            if (result == KFilterBase::End)
                break; // Finished.
        }
        if (result != KFilterBase::Ok)
        {
            bError = true;
            break; // Error
        }
    }

    if (!bError) {
        result = localFile.read(inputBuffer.data(), inputBuffer.size());
        qDebug(KIO_FILTER_DEBUG) << "requestData: got" << result << "(expecting 0)";
        data(QByteArray()); // Send EOF
    }

    filter->terminate();

    if (bError) {
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_READ, subURL.url());
    }

    return KIO::WorkerResult::pass();
}

#include "filter.moc"
