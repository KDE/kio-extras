/*
This file is part of KDE

 Copyright (C) 1999-2000 Waldo Bastian (bastian@kde.org)

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
// $Id$

#include <zlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <kinstance.h>
#include <kdebug.h>
#include <kmimemagic.h>

#include "gzip.h"

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char ** argv)
{
  KInstance instance( "kio_gzip" );

  kdDebug(7110) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_gzip protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  GZipProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7110) << "Done" << endl;
  return 0;
}

GZipProtocol::GZipProtocol( const QCString &pool, const QCString &app )
 : KIO::SlaveBase( "gzip", pool, app )
{ }

void GZipProtocol::get( const KURL & )
{
  if (subURL.isEmpty())
  {
     error( KIO::ERR_NO_SOURCE_PROTOCOL, QString::fromLatin1("gzip") );
     return;
  }
  needSubURLData();

  z_stream zStream;
  zStream.next_in = Z_NULL;
  zStream.avail_in = 0;
  zStream.zalloc = (alloc_func)0;
  zStream.zfree = (free_func)0;
  zStream.opaque = (voidpf)0;

  int result = inflateInit2(&zStream, -MAX_WBITS);

  kdDebug(7110) << "inflateInit returns: " << result << endl;

  bool bNeedHeader = true;
  bool bError = true; // Assume the worst :-)
  bool bNeedMimetype = true;

  QByteArray inputBuffer;
  QByteArray outputBuffer(8*1024); // Start with a modest buffer
  zStream.avail_out = outputBuffer.size();
  zStream.next_out = (Bytef *) outputBuffer.data();
  while(true)
  {
     if (zStream.avail_in == 0)
     {
        dataReq(); // Request data
        result = readData( inputBuffer);
  kdDebug(7110) << "requestData: got " << result << endl;
        if (result <= 0)
        {
          bError = true;
          break; // Unexpected EOF.
        }
        zStream.avail_in = inputBuffer.size();
        zStream.next_in = (Bytef *) inputBuffer.data();
     }
     if (bNeedHeader)
     {
        // Assume the first block of data contains the whole header.
        // The right way is to build this as a big state machine which
        // is a pain in the ass.
        Bytef *p = zStream.next_in;
        int i = zStream.avail_in;
        if ((i -= 10)  < 0) break; // Need at least 10 bytes
        if (*p++ != 0x1f) break; // GZip magic
        if (*p++ != 0x8b) break;
	int method = *p++;
	int flags = *p++;
	if ((method != Z_DEFLATED) || (flags & RESERVED) != 0) break;
	p += 6;
	if ((flags & EXTRA_FIELD) != 0) // skip extra field
	{
	   if ((i -= 2) < 0) break; // Need at least 2 bytes
	   int len = *p++;
	   len += (*p++) << 8;
	   if ((i -= len) < 0) break; // Need at least len bytes
	   p += len;
	}
	if ((flags & ORIG_NAME) != 0) // skip original file name
	{
	   while( (i > 0) && (*p))
	   {
	      i--; p++;
	   }
	   if (--i <= 0) break;
	   p++;
	}
	if ((flags & COMMENT) != 0) // skip comment
	{
	   while( (i > 0) && (*p))
	   {
	      i--; p++;
	   }
	   if (--i <= 0) break;
	   p++;
	}
	if ((flags & HEAD_CRC) != 0) // skipthe header crc
	{
	   if ((i-=2) < 0) break;
	   p += 2;
	}

        zStream.avail_in = i;
        zStream.next_in = p;
	
        bNeedHeader = false;
        bError = false;        
     }
     result = inflate(&zStream, Z_SYNC_FLUSH);
kdDebug(7110) << "inflate returned " << result << endl;
     if ((zStream.avail_out == 0) || (result == Z_STREAM_END)) 
     {
kdDebug(7110) << "avail_out = " << zStream.avail_out << endl;
        if (zStream.avail_out != 0)
        {
            // Discard unused space :-)
            outputBuffer.resize(outputBuffer.size() - zStream.avail_out);
        }
        if (bNeedMimetype)
        {
            KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( outputBuffer, subURL.fileName() );
            kdDebug(7110) << "Emitting mimetype " << result->mimeType() << endl;
            mimeType( result->mimeType() );
            bNeedMimetype = false;
        }
        data( outputBuffer ); // Send data
        zStream.avail_out = outputBuffer.size();
        zStream.next_out = (Bytef *) outputBuffer.data();
        if (result == Z_STREAM_END) 
           break; // Finished.
     }
     if (result != Z_OK)
     {
        bError = true;
        break; // Error     
     }
  }

  if (!bError)
  {
     dataReq(); // Request data
     result = readData( inputBuffer);
  kdDebug(7110) << "requestData: got " << result << "(expecting 0)" << endl;
     data( QByteArray() ); // Send EOF
  }
  
  result = inflateEnd(&zStream);
  kdDebug(7110) << "inflateEnd returned " << result << endl;

  if (bError)
  {
     error(KIO::ERR_COULD_NOT_READ, subURL.url());
     subURL = KURL(); // Clear subURL
     return;
  }

  subURL = KURL(); // Clear subURL
  finished();
}

void GZipProtocol::put( const KURL &url, int, bool _overwrite, bool /*_resume*/ )
{
  error( KIO::ERR_UNSUPPORTED_ACTION, QString::fromLatin1("put"));
}

void GZipProtocol::setSubURL(const KURL &url)
{
   subURL = url;
}

