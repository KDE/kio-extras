/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kgzipdev.h"
#include <kdebug.h>
#include <zlib.h>
#include <assert.h>

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

class KGzipDev::KGzipDevPrivate
{
public:
    KGzipDevPrivate( QIODevice * _dev )
        : dev(_dev)
    {
        zStream.zalloc = (alloc_func)0;
        zStream.zfree = (free_func)0;
        zStream.opaque = (voidpf)0;
    }

    // warning this may be read-only specific
    void init()
    {
        zStream.next_in = Z_NULL;
        zStream.avail_in = 0;
        /*outputBuffer.resize( 8*1024 );// Start with a modest buffer
          zStream.avail_out = outputBuffer.size();
          zStream.next_out = (Bytef *) outputBuffer.data();*/

        int result = inflateInit2(&zStream, -MAX_WBITS);
        kdDebug() << "inflateInit returned " << result << endl;
        // Not idea what to do with result :)

        bNeedHeader = true;
        bCompressed = true;
    }

    // hide away the dirty stuff :)
    bool readHeader()
    {
        // Assume the first block of data contains the whole header.
        // The right way is to build this as a big state machine which
        // is a pain in the ass.
        Bytef *p = zStream.next_in;
        int i = zStream.avail_in;
        if ((i -= 10)  < 0) return false; // Need at least 10 bytes
        if (*p++ != 0x1f) return false; // GZip magic
        if (*p++ != 0x8b) return false;
        int method = *p++;
        int flags = *p++;
        if ((method != Z_DEFLATED) || (flags & RESERVED) != 0) return false;
        p += 6;
        if ((flags & EXTRA_FIELD) != 0) // skip extra field
        {
            if ((i -= 2) < 0) return false; // Need at least 2 bytes
            int len = *p++;
            len += (*p++) << 8;
            if ((i -= len) < 0) return false; // Need at least len bytes
            p += len;
        }
        if ((flags & ORIG_NAME) != 0) // skip original file name
        {
            while( (i > 0) && (*p))
            {
                i--; p++;
            }
            if (--i <= 0) return false;
            p++;
        }
        if ((flags & COMMENT) != 0) // skip comment
        {
            while( (i > 0) && (*p))
            {
                i--; p++;
            }
            if (--i <= 0) return false;
            p++;
        }
        if ((flags & HEAD_CRC) != 0) // skipthe header crc
        {
            if ((i-=2) < 0) return false;
            p += 2;
        }

        zStream.avail_in = i;
        zStream.next_in = p;
        return true;
    }

    // The reason for this stuff here is to avoid including zlib.h in kgzipdev.h
    z_stream zStream;
    QIODevice * dev;
    bool bNeedHeader;
    bool bCompressed;
    QByteArray inputBuffer;
    //QByteArray outputBuffer;
};

KGzipDev::KGzipDev( QIODevice * dev )
{
    assert(dev);
    d = new KGzipDevPrivate(dev);
    // Some setFlags calls are probably missing here,
    // for proper results of the state methods,
    // but the Qt doc says "internal" (??).
}

KGzipDev::~KGzipDev()
{
    delete d;
}

bool KGzipDev::open( int mode )
{
    kdDebug() << "KGzipDev::open " << mode << endl;
    ASSERT( mode == IO_ReadOnly ); // for now
    d->init();
    bool ret = d->dev->open( mode );
    if ( !ret )
        kdWarning() << "Couldn't open underlying device" << endl;
    ioIndex = 0;
    return ret;
}

void KGzipDev::close()
{
    kdDebug() << "KGzipDev::close" << endl;

    // readonly specific !
    int result = inflateEnd(&d->zStream);
    kdDebug() << "inflateEnd returned " << result << endl;

    d->dev->close();
}

void KGzipDev::flush()
{
    kdDebug() << "KGzipDev::flush" << endl;
    d->dev->flush();
}

uint KGzipDev::size() const
{
    // Well, hmm, Houston, we have a problem.
    // We can't know the size of the uncompressed data
    // before uncompressing it.......

    // But readAll, which is not virtual, needs the size.........

    kdWarning() << "KGzipDev::size - can't be implemented !!!!!!!! Returning -1 " << endl;
    return (uint)-1;
}

int KGzipDev::at() const
{
    return ioIndex;
}

bool KGzipDev::at( int pos )
{
    kdDebug() << "KGzipDev::at " << pos << endl;

    if ( ioIndex == pos )
        return true;

    if ( pos == 0 )
    {
        ioIndex = 0;
        // We can forget about the cached data
        d->init();
        return d->dev->reset();
    }

    if ( ioIndex < pos ) // we can start from here
        pos = pos - ioIndex;
    else
    {
        // we have to start from 0 ! Ugly and slow, but better than the previous
        // solution (KTarGz was allocating everything into memory)
        if (!at(0)) // sets ioIndex to 0
            return false;
    }

    kdDebug() << "KGzipDev::at : reading " << pos << " dummy bytes" << endl;
    // #### Slow, and allocate a huge block of memory (potentially)
    // Maybe we could have a flag in the class to know we don't care about the
    // actual data
    QByteArray dummy( pos );
    return ( readBlock( dummy.data(), pos ) == pos ) ;
}

bool KGzipDev::atEnd() const
{
    return d->dev->atEnd();
}

int KGzipDev::readBlock( char *data, uint maxlen )
{
    // Let's hope maxlen is big enough................
    d->zStream.avail_out = maxlen;
    d->zStream.next_out = (Bytef *) data;

    uint dataReceived = 0;
    uint availOut = maxlen;
    int result;
    while ( dataReceived < maxlen )
    {
        if (d->zStream.avail_in == 0)
        {
            d->inputBuffer.resize( 8*1024 /*no idea what to set here*/ );
            // Request data from underlying device
            d->zStream.avail_in = d->dev->readBlock( d->inputBuffer.data(),
                                                     d->inputBuffer.size() );
            d->zStream.next_in = (Bytef*) d->inputBuffer.data();
            kdDebug() << "KGzipDev::readBlock got " << d->zStream.avail_in << " bytes from d->dev" << endl;
        }
        if (d->bNeedHeader)
        {
            if ( ! d->readHeader() )
            {
                kdWarning() << "KGzipDev: Error reading header, assuming non-compressed stream" << endl;
                d->bCompressed = false;
            }
            d->bNeedHeader = false;
        }

        if ( d->bCompressed )
        {
            result = inflate(&d->zStream, Z_SYNC_FLUSH);
            kdDebug() << "inflate returned " << result << endl;
        } else
        {
            // I'm not sure we really need support for that (uncompressed streams),
            // but why not, it can't hurt to have it. One case I can think of is someone
            // naming a tar file "blah.tar.gz" :-)
            if ( d->zStream.avail_in )
            {
                int n = (d->zStream.avail_in < d->zStream.avail_out) ? d->zStream.avail_in : d->zStream.avail_out;
                memcpy( d->zStream.next_out, d->zStream.next_in, n );
                d->zStream.avail_out -= n;
                d->zStream.next_in += n;
                d->zStream.avail_in -= n;
                result = Z_OK;
            } else
                result = Z_STREAM_END;
        }

        // No more space in output buffer, or finished ?
        if ((d->zStream.avail_out == 0) || (result == Z_STREAM_END))
        {
            // We got that much data since the last time we went here
            uint outReceived = availOut - d->zStream.avail_out;

            kdDebug() << "avail_out = " << d->zStream.avail_out << " result=" << result << " outReceived=" << outReceived << endl;

            // Move on in the output buffer
            data += outReceived;
            dataReceived += outReceived;
            ioIndex += outReceived;
            if (result == Z_STREAM_END)
                break; // Finished.
            availOut = maxlen - dataReceived;
            d->zStream.avail_out = availOut;
            d->zStream.next_out = (Bytef *) data;
        }
        if (result != Z_OK)
        {
            kdDebug() << "KGzipDev: result NOT OK" << endl;
            // What to do ?
            break;
        }
    }

    return dataReceived;
}
int KGzipDev::writeBlock( const char *data, uint len )
{
    // not implemented
    (void) data;
    return len - len;
}

int KGzipDev::getch()
{
    kdDebug() << "KGzipDev::getch" << endl;
    return -1;
}

int KGzipDev::putch( int )
{
    kdDebug() << "KGzipDev::putch" << endl;
    return -1;
}

int KGzipDev::ungetch( int )
{
    kdDebug() << "KGzipDev::ungetch" << endl;
    return -1;
}

