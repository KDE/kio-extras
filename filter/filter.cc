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
#include <QFileInfo>
#include <QFile>

#include <kcomponentdata.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kfilterbase.h>
#include <kurl.h>

extern "C" { KDE_EXPORT int kdemain(int argc, char **argv); }

int kdemain( int argc, char ** argv)
{
  KComponentData componentData( "kio_filter" );

  kDebug(7110) << "Starting";

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_filter protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  FilterProtocol slave(argv[1], argv[2], argv[3]);
  slave.dispatchLoop();

  kDebug(7110) << "Done";
  return 0;
}

FilterProtocol::FilterProtocol( const QByteArray & protocol, const QByteArray &pool, const QByteArray &app )
 : KIO::SlaveBase( protocol, pool, app )
{
    QString mimetype = QString::fromLatin1("application/x-") + QString::fromLatin1(protocol);
    filter = KFilterBase::findFilterByMimeType( mimetype );
    Q_ASSERT(filter);
}

void FilterProtocol::get(const KUrl& url)
{
    // In the old solution, subURL would be set by setSubURL.
    // KDE4: now I simply assume bzip2:/localpath/file.bz2 and set subURL to the local path.
    subURL = url;
    subURL.setProtocol("file");

    if (subURL.isEmpty()) {
        error(KIO::ERR_NO_SOURCE_PROTOCOL, QString::fromLatin1(mProtocol));
        return;
    }

    QFile localFile(url.path());
    if (!localFile.open(QIODevice::ReadOnly)) {
        error(KIO::ERR_COULD_NOT_READ, QString::fromLatin1(mProtocol));
        return;
    }

    if (!filter) {
        // TODO better error message
        error(KIO::ERR_INTERNAL, QString::fromLatin1(mProtocol));
        return;
    }

#if 0
  needSubUrlData();
#endif

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
#if 0
        dataReq(); // Request data
        result = readData( inputBuffer);
#else
        result = localFile.read(inputBuffer.data(), inputBuffer.size());
#endif
        kDebug(7110) << "requestData: got " << result;
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
         kDebug(7110) << "avail_out = " << filter->outBufferAvailable();
        if (filter->outBufferAvailable() != 0)
        {
            // Discard unused space :-)
            outputBuffer.resize(outputBuffer.size() - filter->outBufferAvailable());
        }
        if (bNeedMimetype) {
            // Can we use the "base" filename? E.g. foo.txt.bz2
            const QString extension = QFileInfo(subURL.path()).suffix();
            KMimeType::Ptr mime;
            if (extension == "gz" || extension == "bz" || extension == "bz2") {
                QString baseName = subURL.path();
                baseName.truncate(baseName.length() - extension.length() - 1 /*the dot*/);
                kDebug(7110) << "baseName=" << baseName;
                mime = KMimeType::findByNameAndContent(baseName, outputBuffer);
            } else {
                mime = KMimeType::findByContent(outputBuffer);
            }
            kDebug(7110) << "Emitting mimetype " << mime->name();
            mimeType( mime->name() );
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
#if 0
        dataReq(); // Request data
        result = readData( inputBuffer);
#else
        result = localFile.read(inputBuffer.data(), inputBuffer.size());
#endif
        kDebug(7110) << "requestData: got" << result << "(expecting 0)";
        data(QByteArray()); // Send EOF
    }

    filter->terminate();

    if (bError) {
        error(KIO::ERR_COULD_NOT_READ, subURL.url());
    } else {
        finished();
    }
    subURL = KUrl(); // Clear subURL
}

#if 0
void FilterProtocol::put( const KUrl &/*url*/, int, KIO::JobFlags /* _flags */ )
{
  error( KIO::ERR_UNSUPPORTED_ACTION, QString::fromLatin1("put"));
}

void FilterProtocol::setSubURL(const KUrl &url)
{
   subURL = url;
}
#endif
