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
//     a copy from https://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////


#include "kio_smb.h"
#include "kio_smb_internal.h"

#include <QVarLengthArray>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>

//===========================================================================
void SMBSlave::get( const QUrl& kurl )
{
    char        buf[MAX_XFER_BUF_SIZE];
    int         filefd          = 0;
    int         errNum          = 0;
    ssize_t     bytesread       = 0;
    // time_t      curtime         = 0;
    // time_t      lasttime        = 0; // Disabled durint port to Qt5/KF5. Seems to be unused.
    // time_t      starttime       = 0; // Disabled durint port to Qt5/KF5. Seems to be unused.
    KIO::filesize_t totalbytesread  = 0;
    QByteArray  filedata;
    SMBUrl      url;

    qCDebug(KIO_SMB_LOG) << kurl;

    // check (correct) URL
    QUrl kvurl = checkURL(kurl);
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
    errNum = cache_stat(url,&st);
    if( errNum != 0 )
    {
        if ( errNum == EACCES )
           error( KIO::ERR_ACCESS_DENIED, url.toDisplayString());
        else
           error( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
        return;
    }
    if ( S_ISDIR( st.st_mode ) ) {
        error( KIO::ERR_IS_DIRECTORY, url.toDisplayString());
        return;
    }

    // Set the total size
    totalSize( st.st_size );

    // Open and read the file
    filefd = smbc_open(url.toSmbcUrl(),O_RDONLY,0);
    if(filefd >= 0)
    {
        bool isFirstPacket = true;
        // lasttime = starttime = time(NULL); // This seems to be unused..
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
                error( KIO::ERR_CANNOT_READ, url.toDisplayString());
                return;
            }

            filedata = QByteArray::fromRawData(buf,bytesread);
            if (isFirstPacket)
            {
                QMimeDatabase db;
                QMimeType type = db.mimeTypeForFileNameAndData(url.fileName(), filedata);
                mimeType(type.name());
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
          error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString());
	  return;
    }

    finished();
}


//===========================================================================
void SMBSlave::open( const QUrl& kurl, QIODevice::OpenMode mode)
{
    int           errNum = 0;
    qCDebug(KIO_SMB_LOG) << kurl;

    // check (correct) URL
    QUrl kvurl = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (kvurl != kurl) {
        redirection(kvurl);
        finished();
        return;
    }

    if(!auth_initialize_smbc()) {
        error( KIO::ERR_ACCESS_DENIED, kurl.toDisplayString());
        return;
    }

    // Save the URL as a private member
    // FIXME For some reason m_openUrl has be be declared in bottom private
    // section of the class SMBSlave declaration instead of the top section
    // or else this assignment fails
    m_openUrl = kurl;

    // Stat
    errNum = cache_stat(m_openUrl,&st);
    if( errNum != 0 )
    {
        if ( errNum == EACCES )
            error( KIO::ERR_ACCESS_DENIED, m_openUrl.toDisplayString());
        else
            error( KIO::ERR_DOES_NOT_EXIST, m_openUrl.toDisplayString());
        return;
    }
    if ( S_ISDIR( st.st_mode ) ) {
        error( KIO::ERR_IS_DIRECTORY, m_openUrl.toDisplayString());
        return;
    }

    // Set the total size
    totalSize( st.st_size );

    // Convert permissions
    int flags = 0;
    if (mode & QIODevice::ReadOnly) {
        if (mode & QIODevice::WriteOnly) {
            flags = O_RDWR | O_CREAT;
        } else {
            flags = O_RDONLY;
        }
    } else if (mode & QIODevice::WriteOnly) {
        flags = O_WRONLY | O_CREAT;
    }

    if (mode & QIODevice::Append) {
        flags |= O_APPEND;
    } else if (mode & QIODevice::Truncate) {
        flags |= O_TRUNC;
    }

    // Open the file
    m_openFd = smbc_open(m_openUrl.toSmbcUrl(), flags, 0);
    if(m_openFd < 0)
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, m_openUrl.toDisplayString());
        return;
    }

    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all slaves (for KRun/BrowserRun to work).
    // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
    // read the file and send the mimetype.
    if (mode & QIODevice::ReadOnly){
        ssize_t     bytesRequested  = 1024;
        ssize_t     bytesRead       = 0;
        QVarLengthArray<char> buffer(bytesRequested);
        bytesRead = smbc_read(m_openFd, buffer.data(), bytesRequested);
        if(bytesRead < 0)
        {
            error( KIO::ERR_CANNOT_READ, m_openUrl.toDisplayString());
            closeWithoutFinish();
            return;
        }
        else
        {
            QByteArray fileData = QByteArray::fromRawData(buffer.data(),bytesRead);
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(m_openUrl.fileName(), fileData);
            mimeType(type.name());

            off_t res = smbc_lseek(m_openFd, 0, SEEK_SET);
            if (res == (off_t)-1) {
                error(KIO::ERR_CANNOT_SEEK, m_openUrl.path());
                closeWithoutFinish();
                return;
            }
        }
    }

    position( 0 );
    emit opened();
}


void SMBSlave::read( KIO::filesize_t bytesRequested )
{
    Q_ASSERT(m_openFd != -1);

    QVarLengthArray<char> buffer(bytesRequested);
    ssize_t     bytesRead       = 0;

    bytesRead = smbc_read(m_openFd, buffer.data(), bytesRequested);
    Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytesRequested));

    if(bytesRead < 0)
    {
        qCDebug(KIO_SMB_LOG) << "Could not read " << m_openUrl;
        error( KIO::ERR_CANNOT_READ, m_openUrl.toDisplayString());
        closeWithoutFinish();
        return;
    }

    QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
    data( fileData );
}


void SMBSlave::write(const QByteArray &fileData)
{
    Q_ASSERT(m_openFd != -1);

    QByteArray buf(fileData);

    ssize_t size = smbc_write(m_openFd, buf.data(), buf.size());
    if (size < 0)
    {
        qCDebug(KIO_SMB_LOG) << "Could not write to " << m_openUrl;
        error( KIO::ERR_CANNOT_WRITE, m_openUrl.toDisplayString());
        closeWithoutFinish();
        return;
    }

    written(size);
}

void SMBSlave::seek(KIO::filesize_t offset)
{
    off_t res = smbc_lseek(m_openFd, static_cast<off_t>(offset), SEEK_SET);
    if (res == (off_t)-1) {
        error(KIO::ERR_CANNOT_SEEK, m_openUrl.path());
        closeWithoutFinish();
    } else {
        qCDebug(KIO_SMB_LOG) << "res" << res;
        position( res );
    }
}

void SMBSlave::truncate(KIO::filesize_t length)
{
    off_t res = smbc_ftruncate(m_openFd, static_cast<off_t>(length));
    if (res < 0) {
        error(KIO::ERR_CANNOT_TRUNCATE, m_openUrl.path());
        closeWithoutFinish();
    } else {
        qCDebug( KIO_SMB_LOG ) << "res" << res;
        truncated(length);
    }
}

void SMBSlave::closeWithoutFinish()
{
    smbc_close(m_openFd);
}

void SMBSlave::close()
{
    closeWithoutFinish();
    finished();
}

//===========================================================================
void SMBSlave::put( const QUrl& kurl,
                    int permissions,
                    KIO::JobFlags flags )
{

    void *buf;
    size_t bufsize;

    m_current_url = kurl;

    int         filefd;
    bool        exists;
    int         errNum = 0;
    off_t       retValLSeek = 0;
    mode_t      mode;
    QByteArray  filedata;

    qCDebug(KIO_SMB_LOG) << kurl;

    errNum = cache_stat(m_current_url, &st);
    exists = (errNum == 0);
    if ( exists &&  !(flags & KIO::Overwrite) && !(flags & KIO::Resume))
    {
        if (S_ISDIR(st.st_mode))
        {
            qCDebug(KIO_SMB_LOG) << kurl <<" already isdir !!";
            error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.toDisplayString());
        }
        else
        {
            qCDebug(KIO_SMB_LOG) << kurl <<" already exist !!";
            error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.toDisplayString());
        }
        return;
    }

    if (exists && !(flags & KIO::Resume) && (flags & KIO::Overwrite))
    {
        qCDebug(KIO_SMB_LOG) << "exists try to remove " << m_current_url.toSmbcUrl();
        //   remove(m_current_url.url().toLocal8Bit());
    }


    if (flags & KIO::Resume)
    {
        // append if resuming
        qCDebug(KIO_SMB_LOG) << "resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_RDWR, 0 );
        if (filefd < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }

        retValLSeek = smbc_lseek(filefd, 0, SEEK_END);
        if (retValLSeek == (off_t)-1) {
            errNum = errno;
        } else {
            errNum = 0;
        }
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

        qCDebug(KIO_SMB_LOG) << "NO resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (filefd < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    }

    if ( filefd < 0 )
    {
        if ( errNum == EACCES )
        {
            qCDebug(KIO_SMB_LOG) << "error " << kurl <<" access denied !!";
            error( KIO::ERR_WRITE_ACCESS_DENIED, m_current_url.toDisplayString());
        }
        else
        {
            qCDebug(KIO_SMB_LOG) << "error " << kurl <<" can not open for writing !!";
            error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, m_current_url.toDisplayString());
        }
        return;
    }

    // Loop until we got 0 (end of data)
    while(1)
    {
        qCDebug(KIO_SMB_LOG) << "request data ";
        dataReq(); // Request for data
        qCDebug(KIO_SMB_LOG) << "write " << m_current_url.toSmbcUrl();

        if (readData(filedata) <= 0)
        {
            qCDebug(KIO_SMB_LOG) << "readData <= 0";
            break;
        }
        qCDebug(KIO_SMB_LOG) << "write " << m_current_url.toSmbcUrl();
	buf = filedata.data();
	bufsize = filedata.size();
        ssize_t size = smbc_write(filefd, buf, bufsize);
        if ( size < 0)
        {
            qCDebug(KIO_SMB_LOG) << "error " << kurl << "could not write !!";
            error( KIO::ERR_CANNOT_WRITE, m_current_url.toDisplayString());
            return;
        }
        qCDebug(KIO_SMB_LOG) << "wrote " << size;
    }
    qCDebug(KIO_SMB_LOG) << "close " << m_current_url.toSmbcUrl();

    if(smbc_close(filefd) < 0)
    {
        qCDebug(KIO_SMB_LOG) << kurl << "could not write !!";
        error( KIO::ERR_CANNOT_WRITE, m_current_url.toDisplayString());
        return;
    }

    // set final permissions, if the file was just created
    if ( permissions != -1 && !exists )
    {
        // TODO: did the smbc_chmod fail?
        // TODO: put in call to chmod when it is working!
        // smbc_chmod(url.toSmbcUrl(),permissions);
    }
#ifdef HAVE_UTIME_H
    // set modification time
    const QString mtimeStr = metaData( "modified" );
    if ( !mtimeStr.isEmpty() ) {
        QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
        if ( dt.isValid() ) {
            if (cache_stat( m_current_url, &st ) == 0) {
                struct utimbuf utbuf;
                utbuf.actime = st.st_atime; // access time, unchanged
                utbuf.modtime = dt.toSecsSinceEpoch(); // modification time
                smbc_utime( m_current_url.toSmbcUrl(), &utbuf );
            }
        }
    }
#endif

    // We have done our job => finish
    finished();
}




