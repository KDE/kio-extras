/*
This file is part of KDE

 Copyright (C) 1999-2000 Waldo Bastian (bastian@kde.org)
 Copyright 2008 David Faure <faure@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "filter.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QUrl>
#include <QMimeType>
#include <QMimeDatabase>

#include <kfilterbase.h>
#include <kcompressiondevice.h>
#include <kfilterdev.h>

#include "loggingcategory.h"

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

    FilterProtocol slave(argv[1], argv[2], argv[3]);
    slave.dispatchLoop();

    qDebug(KIO_FILTER_DEBUG) << "Done";
    return 0;
}

FilterProtocol::FilterProtocol( const QByteArray & protocol, const QByteArray &pool, const QByteArray &app )
    : KIO::SlaveBase( protocol, pool, app )
{
    QString mimetype = QString::fromLatin1("application/x-") + QString::fromLatin1(protocol);
    filter = KCompressionDevice::filterForCompressionType(KFilterDev::compressionTypeForMimeType( mimetype ));
    Q_ASSERT(filter);
}

void FilterProtocol::get(const QUrl& url)
{
    // In the old solution, subURL would be set by setSubURL.
    // KDE4: now I simply assume bzip2:/localpath/file.bz2 and set subURL to the local path.
    subURL = url;
    subURL.setScheme("file");

    if (subURL.isEmpty()) {
        error(KIO::ERR_NO_SOURCE_PROTOCOL, QString::fromLatin1(mProtocol));
        return;
    }

    QFile localFile(url.path());
    if (!localFile.open(QIODevice::ReadOnly)) {
        error(KIO::ERR_CANNOT_READ, QString::fromLatin1(mProtocol));
        return;
    }

    if (!filter) {
        // TODO better error message
        error(KIO::ERR_INTERNAL, QString::fromLatin1(mProtocol));
        return;
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
                if (extension == "gz" || extension == "bz" || extension == "bz2") {
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
        error(KIO::ERR_CANNOT_READ, subURL.url());
    } else {
        finished();
    }
    subURL = QUrl(); // Clear subURL
}
