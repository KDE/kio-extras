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
#include "kgzipfilter.h"
#include <kdebug.h>
#include <stdio.h> // for EOF
#include <assert.h>

class KGzipDev::KGzipDevPrivate
{
public:
    bool bNeedHeader;
    QByteArray inputBuffer;
    QCString ungetchBuffer;

    //QByteArray outputBuffer;
};

KGzipDev::KGzipDev( QIODevice * dev )
{
    assert(dev);
    d = new KGzipDevPrivate;
    filter = new KGzipFilter(dev);
    // Some setFlags calls are probably missing here,
    // for proper results of the state methods,
    // but the Qt doc says "internal" (??).
}

KGzipDev::~KGzipDev()
{
    delete filter;
    delete d;
}

bool KGzipDev::open( int mode )
{
    kdDebug() << "KGzipDev::open " << mode << endl;
    ASSERT( mode == IO_ReadOnly ); // for now
    filter->init();
    bool ret = filter->device()->open( mode );
    d->ungetchBuffer.resize(0);
    d->bNeedHeader = true;
    if ( !ret )
        kdWarning() << "Couldn't open underlying device" << endl;
    ioIndex = 0;
    return ret;
}

void KGzipDev::close()
{
    kdDebug() << "KGzipDev::close" << endl;

    filter->device()->close();
    filter->terminate();
}

void KGzipDev::flush()
{
    kdDebug() << "KGzipDev::flush" << endl;
    filter->device()->flush();
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
        d->ungetchBuffer.resize(0);
        d->bNeedHeader = true;
        filter->setInBuffer(0L,0);
        filter->reset();
        return filter->device()->reset();
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
    return filter->device()->atEnd();
}

int KGzipDev::readBlock( char *data, uint maxlen )
{
    filter->setOutBuffer( data, maxlen );

    uint dataReceived = 0;
    uint availOut = maxlen;
    KFilterBase::Result result;
    while ( dataReceived < maxlen )
    {
        if (filter->inBufferEmpty())
        {
            // Not sure about the best size to set there.
            // For sure, it should be bigger than the header size (see comment in readHeader)
            d->inputBuffer.resize( 8*1024 );
            // Request data from underlying device
            int size = filter->device()->readBlock( d->inputBuffer.data(),
                                                    d->inputBuffer.size() );
            filter->setInBuffer( d->inputBuffer.data(), size );
            kdDebug() << "KGzipDev::readBlock got " << size << " bytes from device" << endl;
        }
        if (d->bNeedHeader)
        {
            (void) filter->readHeader();
            d->bNeedHeader = false;
        }

        result = filter->uncompress();

        if (result == KFilterBase::ERROR)
        {
            kdDebug() << "KGzipDev: Error when uncompressing data" << endl;
            // What to do ?
            break;
        }

        // No more space in output buffer, or finished ?
        if ((filter->outBufferFull()) || (result == KFilterBase::END))
        {
            // We got that much data since the last time we went here
            uint outReceived = availOut - filter->outBufferAvailable();

            //kdDebug() << "avail_out = " << filter->outBufferAvailable() << " result=" << result << " outReceived=" << outReceived << endl;

            // Move on in the output buffer
            data += outReceived;
            dataReceived += outReceived;
            ioIndex += outReceived;
            if (result == KFilterBase::END)
                break; // Finished.
            availOut = maxlen - dataReceived;
            filter->setOutBuffer( data, availOut );
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
    if ( !d->ungetchBuffer.isEmpty() ) {
        int len = d->ungetchBuffer.length();
        int ch = d->ungetchBuffer[ len-1 ];
        d->ungetchBuffer.truncate( len - 1 );
        return ch;
    }
    char buf[1];
    return readBlock( buf, 1 ) == 1 ? buf[0] : EOF;
}

int KGzipDev::putch( int )
{
    kdDebug() << "KGzipDev::putch" << endl;
    return -1;
}

int KGzipDev::ungetch( int ch )
{
    kdDebug() << "KGzipDev::ungetch" << endl;
    if ( ch == EOF )                            // cannot unget EOF
        return ch;

    // pipe or similar => we cannot ungetch, so do it manually
    d->ungetchBuffer +=ch;
    return ch;
}

