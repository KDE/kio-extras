////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_file.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              SMB file access
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program; see the file COPYING.  If not, please obtain
//     a copy from http://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////


#include "kio_smb.h"
#include "kio_smb_internal.h"

#include <QVarLengthArray>

#include <kmimetype.h>

//===========================================================================
void SMBSlave::get( const KUrl& kurl )
{
    char        buf[MAX_XFER_BUF_SIZE];
    int         filefd          = 0;
    ssize_t     bytesread       = 0;
    // time_t      curtime         = 0;
    time_t      lasttime        = 0;
    time_t      starttime       = 0;
    KIO::filesize_t totalbytesread  = 0;
    QByteArray  filedata;
    SMBUrl      url;

    kDebug(KIO_SMB) << "SMBSlave::get on " << kurl;

    // check (correct) URL
    KUrl kvurl = checkURL(kurl);
    // if URL is not valid we have to redirect to correct URL
    if (kvurl != kurl) {
        redirection(kvurl);
        finished();
        return;
    }

    if(!auth_initialize_smbc())
        return;

    // Stat
    url = kurl;
    if(cache_stat(url,&st) == -1 )
    {
        if ( errno == EACCES )
           error( KIO::ERR_ACCESS_DENIED, url.prettyUrl());
        else
           error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
        return;
    }
    if ( S_ISDIR( st.st_mode ) ) {
        error( KIO::ERR_IS_DIRECTORY, url.prettyUrl());
        return;
    }

    // Set the total size
    totalSize( st.st_size );

    // Open and read the file
    filefd = smbc_open(url.toSmbcUrl(),O_RDONLY,0);
    if(filefd >= 0)
    {
        bool isFirstPacket = true;
        lasttime = starttime = time(NULL);
        while(1)
        {
            bytesread = smbc_read(filefd, buf, MAX_XFER_BUF_SIZE);
            if(bytesread == 0)
            {
                // All done reading
                break;
            }
            else if(bytesread < 0)
            {
                error( KIO::ERR_COULD_NOT_READ, url.prettyUrl());
                return;
            }

            filedata.setRawData(buf,bytesread);
            if (isFirstPacket)
            {
                KMimeType::Ptr p_mimeType = KMimeType::findByNameAndContent(url.fileName(), filedata);
                mimeType(p_mimeType->name());
                isFirstPacket = false;
            }
            data( filedata );
            filedata.clear();

            // increment total bytes read
            totalbytesread += bytesread;

            processedSize(totalbytesread);
        }

        smbc_close(filefd);
        data( QByteArray() );
        processedSize(static_cast<KIO::filesize_t>(st.st_size));

    }
    else
    {
          error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
	  return;
    }

    finished();
}


//===========================================================================
void SMBSlave::open( const KUrl& kurl, QIODevice::OpenMode /*mode*/)
{
    SMBUrl      url;

    kDebug(KIO_SMB) << "SMBSlave::open on " << kurl;

    // check (correct) URL
    KUrl kvurl = checkURL(kurl);
    // if URL is not valid we have to redirect to correct URL
    if (kvurl != kurl) {
        redirection(kvurl);
        finished();
        return;
    }

    if(!auth_initialize_smbc())
        return;

    // Stat
    url = kurl;
    if(cache_stat(url,&st) == -1 )
    {
        if ( errno == EACCES )
           error( KIO::ERR_ACCESS_DENIED, url.prettyUrl());
        else
           error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
        return;
    }
    if ( S_ISDIR( st.st_mode ) ) {
        error( KIO::ERR_IS_DIRECTORY, url.prettyUrl());
        return;
    }

    // Set the total size
    totalSize( st.st_size );

    // Open and read the file
    openFd = smbc_open(url.toSmbcUrl(),O_RDONLY,0);
    if(openFd < 0)
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
        return;
    }

    position( 0 );
    emit opened();
    openUrl = url;
    justOpened = true;
}

void SMBSlave::read(KIO::filesize_t bytes)
{
    kDebug( KIO_SMB ) << "open -- read";
    Q_ASSERT(openFd != -1);

    QVarLengthArray<char> buffer(bytes);
    ssize_t     bytesread       = 0;
    // time_t      curtime         = 0;
    time_t      lasttime        = 0;
    time_t      starttime       = 0;

    lasttime = starttime = time(NULL);
    bytesread = smbc_read(openFd, buffer.data(), bytes);
    if(bytesread == 0)
    {
        // All done reading
        data(QByteArray());
    }
    else if(bytesread < 0)
    {
        error( KIO::ERR_COULD_NOT_READ, openUrl.prettyUrl());
        close();
        return;
    }
    else
    {
        QByteArray  filedata;
        filedata.setRawData(buffer.data(),bytesread);
        if (justOpened)
        {
            KMimeType::Ptr p_mimeType = KMimeType::findByNameAndContent(openUrl.fileName(), filedata);
            mimeType(p_mimeType->name());
            justOpened = false;
        }
        data( filedata );
        filedata.clear();
    }
}

void SMBSlave::write(const QByteArray &data)
{
    kDebug( KIO_SMB ) << "open -- write";

    // not implemented
    KIO::SlaveBase::write(data);
}

void SMBSlave::seek(KIO::filesize_t offset)
{
    kDebug( KIO_SMB ) << "open -- seek";
    int res = smbc_lseek(openFd, offset, SEEK_SET);
    if (res != -1) {
        position( offset );
    } else {
        error(KIO::ERR_COULD_NOT_SEEK, openUrl.path());
        close();
    }
}

void SMBSlave::close()
{
    smbc_close(openFd);
    finished();
}

//===========================================================================
void SMBSlave::put( const KUrl& kurl,
                    int permissions,
                    KIO::JobFlags flags )
{

    void *buf;
    size_t bufsize;

    m_current_url = kurl;

    int         filefd;
    bool        exists;
    mode_t      mode;
    QByteArray  filedata;

    kDebug(KIO_SMB) << "SMBSlave::put on " << kurl;


    exists = (cache_stat(m_current_url, &st) != -1 );
    if ( exists &&  !(flags & KIO::Overwrite) && !(flags & KIO::Resume))
    {
        if (S_ISDIR(st.st_mode))
        {
            kDebug(KIO_SMB) << "SMBSlave::put on " << kurl <<" already isdir !!";
            error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.prettyUrl());
        }
        else
        {
            kDebug(KIO_SMB) << "SMBSlave::put on " << kurl <<" already exist !!";
            error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.prettyUrl());
        }
        return;
    }

    if (exists && !(flags & KIO::Resume) && (flags & KIO::Overwrite))
    {
        kDebug(KIO_SMB) << "SMBSlave::put exists try to remove " << m_current_url.toSmbcUrl();
        //   remove(m_current_url.url().toLocal8Bit());
    }


    if (flags & KIO::Resume)
    {
        // append if resuming
        kDebug(KIO_SMB) << "SMBSlave::put resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_RDWR, 0 );
        smbc_lseek(filefd, 0, SEEK_END);
    }
    else
    {
        if (permissions != -1)
        {
            mode = permissions | S_IWUSR | S_IRUSR;
        }
        else
        {
            mode = 600;//0666;
        }

        kDebug(KIO_SMB) << "SMBSlave::put NO resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
    }

    if ( filefd < 0 )
    {
        if ( errno == EACCES )
        {
            kDebug(KIO_SMB) << "SMBSlave::put error " << kurl <<" access denied !!";
            error( KIO::ERR_WRITE_ACCESS_DENIED, m_current_url.prettyUrl());
        }
        else
        {
            kDebug(KIO_SMB) << "SMBSlave::put error " << kurl <<" can not open for writing !!";
            error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, m_current_url.prettyUrl());
        }
        finished();
        return;
    }

    // Loop until we got 0 (end of data)
    while(1)
    {
        kDebug(KIO_SMB) << "SMBSlave::put request data ";
        dataReq(); // Request for data
        kDebug(KIO_SMB) << "SMBSlave::put write " << m_current_url.toSmbcUrl();

        if (readData(filedata) <= 0)
        {
            kDebug(KIO_SMB) << "readData <= 0";
            break;
        }
        kDebug(KIO_SMB) << "SMBSlave::put write " << m_current_url.toSmbcUrl();
	buf = filedata.data();
	bufsize = filedata.size();
        int size = smbc_write(filefd, buf, bufsize);
        if ( size < 0)
        {
            kDebug(KIO_SMB) << "SMBSlave::put error " << kurl <<" could not write !!";
            error( KIO::ERR_COULD_NOT_WRITE, m_current_url.prettyUrl());
            finished();
            return;
        }
        kDebug(KIO_SMB ) << "wrote " << size;
    }
    kDebug(KIO_SMB) << "SMBSlave::put close " << m_current_url.toSmbcUrl();

    if(smbc_close(filefd))
    {
        kDebug(KIO_SMB) << "SMBSlave::put on " << kurl <<" could not write !!";
        error( KIO::ERR_COULD_NOT_WRITE, m_current_url.prettyUrl());
        finished();
        return;
    }

    // set final permissions, if the file was just created
    if ( permissions != -1 && !exists )
    {
        // TODO: did the smbc_chmod fail?
        // TODO: put in call to chmod when it is working!
        // smbc_chmod(url.toSmbcUrl(),permissions);
    }

    // We have done our job => finish
    finished();
}




