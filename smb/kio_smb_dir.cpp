/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_dir.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              SMB directory access
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
////---------------------------------------------------------------------------
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

#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include <kconfiggroup.h>
#include <kio/ioslave_defaults.h>

//===========================================================================
void SMBSlave::copy(const QUrl& src, const QUrl& dst, int permissions, KIO::JobFlags flags)
{
    const bool isSourceLocal = src.isLocalFile();
    const bool isDestinationLocal = dst.isLocalFile();

    if (!isSourceLocal && isDestinationLocal) {
        smbCopyGet(src, dst, permissions, flags);
    } else if (isSourceLocal && !isDestinationLocal) {
        smbCopyPut(src, dst, permissions, flags);
    } else {
        smbCopy(src, dst, permissions, flags);
    }
}

void SMBSlave::smbCopy(const QUrl& ksrc, const QUrl& kdst, int permissions, KIO::JobFlags flags)
{

    SMBUrl          src;
    SMBUrl          dst;
    mode_t          initialmode;
    ssize_t         n;
    int             dstflags;
    int             srcfd = -1;
    int             dstfd = -1;
    int             errNum = 0;
    KIO::filesize_t processed_size = 0;
    unsigned char   buf[MAX_XFER_BUF_SIZE];

    qCDebug(KIO_SMB_LOG) << "SMBSlave::copy with src = " << ksrc << "and dest = " << kdst;

    // setup urls
    src = ksrc;
    dst = kdst;

    // Obtain information about source
    errNum = cache_stat(src, &st );
    if( errNum != 0 )
    {
        if ( errNum == EACCES )
        {
            error( KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        }
        else
        {
             error( KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }
    if ( S_ISDIR( st.st_mode ) )
    {
        error( KIO::ERR_IS_DIRECTORY, src.toDisplayString() );
        return;
    }
    totalSize(st.st_size);

    // Check to se if the destination exists
    errNum = cache_stat(dst, &st);
    if( errNum == 0 )
    {
        if(S_ISDIR(st.st_mode))
        {
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.toDisplayString());
	    return;
        }
        if(!(flags & KIO::Overwrite))
        {
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.toDisplayString());
	    return;
	}
    }

    // Open the source file
    srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if (srcfd < 0){
        errNum = errno;
    } else {
        errNum = 0;
    }

    if(srcfd < 0)
    {
        if(errNum == EACCES)
        {
            error( KIO::ERR_ACCESS_DENIED, src.toDisplayString() );
        }
        else
        {
            error( KIO::ERR_DOES_NOT_EXIST, src.toDisplayString() );
        }
	return;
    }

    // Determine initial creation mode
    if(permissions != -1)
    {
        initialmode = permissions | S_IWUSR;
    }
    else
    {
        initialmode = 0 | S_IWUSR;//0666;
    }


    // Open the destination file
    dstflags = O_CREAT | O_TRUNC | O_WRONLY;
    if(!(flags & KIO::Overwrite))
    {
        dstflags |= O_EXCL;
    }
    dstfd = smbc_open(dst.toSmbcUrl(), dstflags, initialmode);
    if (dstfd < 0){
        errNum = errno;
    } else {
        errNum = 0;
    }

    if(dstfd < 0)
    {
        if(errNum == EACCES)
        {
            error(KIO::ERR_WRITE_ACCESS_DENIED, dst.toDisplayString());
        }
        else
        {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, dst.toDisplayString());
        }

        if(srcfd >= 0 )
        {
            smbc_close(srcfd);
        }
        return;
    }


    // Perform copy
    while (true) {
        n = smbc_read(srcfd, buf, MAX_XFER_BUF_SIZE );
        if(n > 0)
        {
            n = smbc_write(dstfd, buf, n);
            if(n == -1)
            {
	        qCDebug(KIO_SMB_LOG) << "SMBSlave::copy copy now KIO::ERR_CANNOT_WRITE";
                error( KIO::ERR_CANNOT_WRITE, dst.toDisplayString());
                break;
            }

            processed_size += n;
	    processedSize(processed_size);
	}
        else if(n == 0)
	{
	      break; // finished
	}
	else
	{
            error( KIO::ERR_CANNOT_READ, src.toDisplayString());
	    break;
        }
    }


    //    FINISHED:

    if(srcfd >= 0 )
    {
        smbc_close(srcfd);
    }

    if(dstfd >= 0)
    {
        if(smbc_close(dstfd) == 0)
        {

            // TODO: set final permissions
        }
        else
        {
            error( KIO::ERR_CANNOT_WRITE, dst.toDisplayString());
	    return;
        }
    }

    finished();
}

void SMBSlave::smbCopyGet(const QUrl& ksrc, const QUrl& kdst, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SMB_LOG) << "src = " << ksrc << ", dest = " << kdst;

    // check if destination is ok ...
    const QString dstFile = kdst.toLocalFile();
    const QFileInfo dstInfo (dstFile);

    if(dstInfo.exists())  {
        if(dstInfo.isDir()) {
            error (ERR_IS_DIRECTORY, kdst.toDisplayString());
            return;
        }

        if(!(flags & KIO::Overwrite)) {
            error(ERR_FILE_ALREADY_EXIST, kdst.toDisplayString());
            return;
        }
    }

    bool bResume = false;
    const QFileInfo partInfo (dstFile + QLatin1String(".part"));
    const bool bPartExists = partInfo.exists();
    const bool bMarkPartial = configValue(QStringLiteral("MarkPartial"), true);

    if (bMarkPartial && bPartExists && partInfo.size() > 0) {
      if (partInfo.isDir()) {
        error(ERR_IS_DIRECTORY, partInfo.absoluteFilePath());
        return;
      }
      bResume = canResume(partInfo.size());
    }

    if (bPartExists && !bResume)                  // get rid of an unwanted ".part" file
      QFile::remove(partInfo.absoluteFilePath());

    // open the output file...
    QFile::OpenMode mode;
    QString filename;
    if (bResume) {
        filename = partInfo.absoluteFilePath();
        mode = QFile::WriteOnly | QFile::Append;
    }
    else {
        filename = (bMarkPartial ? partInfo.absoluteFilePath() : dstFile);
        mode = QFile::WriteOnly | QFile::Truncate;
    }

    QFile file (filename);
    if (!bResume) {
        QFile::Permissions perms;
        if (permissions == -1) {
            perms = QFile::ReadOwner | QFile::WriteOwner;
        } else {
            perms = KIO::convertPermissions(permissions | QFile::WriteOwner);
        }
        file.setPermissions(perms);
    }

    if (!file.open(mode)) {
        qCDebug(KIO_SMB_LOG) << "could not write to" << dstFile;
        switch (file.error()) {
          case QFile::OpenError:
              if (bResume) {
                error (ERR_CANNOT_RESUME, kdst.toDisplayString());
              } else {
                error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.toDisplayString());
              }
              break;
          case QFile::PermissionsError:
              error(ERR_WRITE_ACCESS_DENIED, kdst.toDisplayString());
              break;
          default:
              error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.toDisplayString());
              break;
        }
        return;
    }

    // setup the source urls
    const SMBUrl src(ksrc);

    // Obtain information about source
    int errNum = cache_stat (src, &st);
    if (errNum != 0) {
        if (errNum == EACCES) {
            error (KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        } else {
            error (KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }

    if (S_ISDIR( st.st_mode )) {
        error (KIO::ERR_IS_DIRECTORY, src.toDisplayString());
        return;
    }
    totalSize(st.st_size);

    // Open the source file
    KIO::filesize_t processed_size = 0;
    int srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if (srcfd < 0){
        errNum = errno;
    } else {
        errNum = 0;
        if (bResume) {
            qCDebug(KIO_SMB_LOG) << "seeking to size" << partInfo.size();
            off_t offset = smbc_lseek(srcfd, partInfo.size(), SEEK_SET);
            if (offset == -1) {
                error(KIO::ERR_CANNOT_SEEK, src.toDisplayString());
                smbc_close(srcfd);
                return;
            } else {
                processed_size += offset;
            }
        }
    }

    if (srcfd < 0) {
        if(errNum == EACCES) {
            error( KIO::ERR_ACCESS_DENIED, src.toDisplayString() );
        } else {
            error( KIO::ERR_DOES_NOT_EXIST, src.toDisplayString() );
        }
        return;
    }

    // Perform the copy
    char buf[MAX_XFER_BUF_SIZE];
    bool isErr = false;

    while (true) {
        const ssize_t bytesRead = smbc_read(srcfd, buf, MAX_XFER_BUF_SIZE);
        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                error( KIO::ERR_CANNOT_READ, src.toDisplayString());
                isErr = true;
            }
            break;
        }

        const qint64 bytesWritten = file.write(buf, bytesRead);
        if (bytesWritten == -1) {
            qCDebug(KIO_SMB_LOG) << "copy now KIO::ERR_CANNOT_WRITE";
            error( KIO::ERR_CANNOT_WRITE, kdst.toDisplayString());
            isErr = true;
            break;
        }

        processed_size += bytesWritten;
        processedSize(processed_size);
    }

    // FINISHED
    smbc_close(srcfd);

    // Handle error condition.
    if (isErr) {
        const QString sPart = partInfo.absoluteFilePath();
        if (bMarkPartial) {
            const int size = configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            if (partInfo.size() <  size) {
                QFile::remove(sPart);
            }
        }
        return;
    }

    // Rename partial file to its original name.
    if (bMarkPartial) {
        const QString sPart = partInfo.absoluteFilePath();
        // Remove old dest file if it exists..
        if (dstInfo.exists()) {
            QFile::remove(dstFile);
        }
        if (!QFile::rename(sPart, dstFile)) {
            qCDebug(KIO_SMB_LOG) << "failed to rename" << sPart << "to" << dstFile;
            error(ERR_CANNOT_RENAME_PARTIAL, sPart);
            return;
        }
    }

    // Restore the mtime on the file.
    const QString mtimeStr = metaData("modified");
    qCDebug(KIO_SMB_LOG) << "modified:" << mtimeStr;
    if (!mtimeStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dt.isValid()) {
            struct utimbuf utbuf{};
            utbuf.actime = QFileInfo(dstFile).lastRead().toSecsSinceEpoch(); // access time, unchanged
            utbuf.modtime = dt.toSecsSinceEpoch(); // modification time
            utime(QFile::encodeName(dstFile).constData(), &utbuf);
        }
    }

    finished();
}

void SMBSlave::smbCopyPut(const QUrl& ksrc, const QUrl& kdst, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SMB_LOG) << "src = " << ksrc << ", dest = " << kdst;

    QFile srcFile (ksrc.toLocalFile());
    const QFileInfo srcInfo (srcFile);

    if (srcInfo.exists()) {
        if (srcInfo.isDir()) {
            error(KIO::ERR_IS_DIRECTORY, ksrc.toDisplayString());
            return;
        }
    } else {
        error(KIO::ERR_DOES_NOT_EXIST, ksrc.toDisplayString());
        return;
    }

    if (!srcFile.open(QFile::ReadOnly)) {
        qCDebug(KIO_SMB_LOG) << "could not read from" << ksrc;
        switch (srcFile.error()) {
          case QFile::PermissionsError:
              error(KIO::ERR_WRITE_ACCESS_DENIED, ksrc.toDisplayString());
              break;
          case QFile::OpenError:
          default:
              error(KIO::ERR_CANNOT_OPEN_FOR_READING, ksrc.toDisplayString());
              break;
        }
        return;
    }

    totalSize(static_cast<filesize_t>(srcInfo.size()));

    bool bResume = false;
    bool bPartExists = false;
    const bool bMarkPartial = configValue(QStringLiteral("MarkPartial"), true);
    const SMBUrl dstOrigUrl (kdst);

    if (bMarkPartial) {
        const int errNum = cache_stat(dstOrigUrl.partUrl(), &st);
        bPartExists = (errNum == 0);
        if (bPartExists) {
            if (!(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
                bResume = canResume(st.st_size);
            } else {
                bResume = (flags & KIO::Resume);
            }
        }
    }

    int dstfd = -1;
    int errNum = cache_stat(dstOrigUrl, &st);

    if (errNum == 0 && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
        if (S_ISDIR(st.st_mode)) {
            error( KIO::ERR_IS_DIRECTORY, dstOrigUrl.toDisplayString());
        } else {
            error( KIO::ERR_FILE_ALREADY_EXIST, dstOrigUrl.toDisplayString());
        }
        return;
    }

    KIO::filesize_t processed_size = 0;
    const SMBUrl dstUrl(bMarkPartial ? dstOrigUrl.partUrl() : dstOrigUrl);

    if (bResume) {
        // append if resuming
        qCDebug(KIO_SMB_LOG) << "resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_RDWR, 0 );
        if (dstfd < 0) {
            errNum = errno;
        } else {
            const off_t offset = smbc_lseek(dstfd, 0, SEEK_END);
            if (offset == (off_t)-1) {
                error(KIO::ERR_CANNOT_SEEK, dstUrl.toDisplayString());
                smbc_close(dstfd);
                return;
            } else {
                processed_size = offset;
            }
        }
    } else {
        mode_t mode;
        if (permissions == -1) {
            mode = 600;
        } else {
            mode = permissions | S_IRUSR | S_IWUSR;
        }

        qCDebug(KIO_SMB_LOG) << "NO resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (dstfd < 0) {
            errNum = errno;
        }
    }

    if (dstfd < 0) {
        if (errNum == EACCES) {
            qCDebug(KIO_SMB_LOG) << "access denied";
            error( KIO::ERR_WRITE_ACCESS_DENIED, dstUrl.toDisplayString());
        }
        else {
            qCDebug(KIO_SMB_LOG) << "can not open for writing";
            error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dstUrl.toDisplayString());
        }
        return;
    }

    bool isErr = false;

    if (processed_size == 0 || srcFile.seek(processed_size)) {
        // Perform the copy
        char buf[MAX_XFER_BUF_SIZE];

        while (true) {
            const ssize_t bytesRead = srcFile.read(buf, MAX_XFER_BUF_SIZE);
            if (bytesRead <= 0) {
                if (bytesRead < 0) {
                    error(KIO::ERR_CANNOT_READ, ksrc.toDisplayString());
                    isErr = true;
                }
                break;
            }

            const qint64 bytesWritten = smbc_write(dstfd, buf, bytesRead);
            if (bytesWritten == -1) {
                error(KIO::ERR_CANNOT_WRITE, kdst.toDisplayString());
                isErr = true;
                break;
            }

            processed_size += bytesWritten;
            processedSize(processed_size);
        }
    } else {
        isErr = true;
        error(KIO::ERR_CANNOT_SEEK, ksrc.toDisplayString());
    }

    // FINISHED
    if (smbc_close(dstfd) < 0) {
        qCDebug(KIO_SMB_LOG) << dstUrl << "could not write";
        error( KIO::ERR_CANNOT_WRITE, dstUrl.toDisplayString());
        return;
    }

    // Handle error condition.
    if (isErr) {
        if (bMarkPartial) {
            const int size = configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            const int errNum = cache_stat(dstUrl, &st);
            if (errNum == 0 && st.st_size < size) {
                smbc_unlink(dstUrl.toSmbcUrl());
            }
        }
        return;
    }

    // Rename partial file to its original name.
    if (bMarkPartial) {
        smbc_unlink(dstOrigUrl.toSmbcUrl());
        if (smbc_rename(dstUrl.toSmbcUrl(), dstOrigUrl.toSmbcUrl()) < 0) {
            qCDebug(KIO_SMB_LOG) << "failed to rename" << dstUrl << "to" << dstOrigUrl << "->" << strerror(errno);
            error(ERR_CANNOT_RENAME_PARTIAL, dstUrl.toDisplayString());
            return;
        }
    }

#ifdef HAVE_UTIME_H
    // set modification time
    const QString mtimeStr = metaData( "modified" );
    if (!mtimeStr.isEmpty() ) {
        QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
        if ( dt.isValid() ) {
            struct utimbuf utbuf{};
            utbuf.actime = st.st_atime; // access time, unchanged
            utbuf.modtime = dt.toSecsSinceEpoch(); // modification time
            smbc_utime( dstOrigUrl.toSmbcUrl(), &utbuf );
        }
    }
#endif

    // We have done our job => finish
    finished();
}

//===========================================================================
void SMBSlave::del( const QUrl &kurl, bool isfile)
{
    qCDebug(KIO_SMB_LOG) << kurl;
    m_current_url = kurl;
    int errNum = 0;
    int retVal = 0;

    if(isfile)
    {
        // Delete file
        qCDebug(KIO_SMB_LOG) << "Deleting file" << kurl;
        retVal = smbc_unlink(m_current_url.toSmbcUrl());
        if ( retVal < 0 ){
            errNum = errno;
        } else {
            errNum = 0;
        }
    }
    else
    {
        qCDebug(KIO_SMB_LOG) << "Deleting directory" << kurl;
        // Delete directory
        retVal = smbc_rmdir(m_current_url.toSmbcUrl());
        if( retVal < 0 ) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    }

    if( errNum != 0 )
    {
        reportError(kurl, errNum);
    }
    else
    {
        finished();
    }
}

//===========================================================================
void SMBSlave::mkdir( const QUrl &kurl, int permissions )
{
    qCDebug(KIO_SMB_LOG) << kurl;
    int errNum = 0;
    int retVal = 0;
    m_current_url = kurl;

    retVal = smbc_mkdir(m_current_url.toSmbcUrl(), 0777);
    if( retVal < 0 ){
        errNum = errno;
    } else {
        errNum = 0;
    }

    if( retVal < 0 )
    {
        if (errNum == EEXIST) {
            errNum = cache_stat(m_current_url, &st );
            if (errNum == 0 && S_ISDIR(st.st_mode))
            {
                error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.toDisplayString());
            }
            else
            {
                error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.toDisplayString());
            }
        }
        else
        {
            reportError(kurl, errNum);
        }
        qCDebug(KIO_SMB_LOG) << "exit with error " << kurl;
    }
    else // success
    {
        if(permissions != -1)
        {
            // TODO enable the following when complete
            //smbc_chmod( url.toSmbcUrl(), permissions );
        }
        finished();
    }
}


//===========================================================================
void SMBSlave::rename( const QUrl& ksrc, const QUrl& kdest, KIO::JobFlags flags )
{

    SMBUrl      src;
    SMBUrl      dst;
    int         errNum = 0;
    int         retVal = 0;

    qCDebug(KIO_SMB_LOG) << "old name = " << ksrc << ", new name = " << kdest;

    src = ksrc;
    dst = kdest;

    // Check to se if the destination exists

    qCDebug(KIO_SMB_LOG) << "stat dst";
    errNum = cache_stat(dst, &st);
    if( errNum == 0 )
    {
        if(S_ISDIR(st.st_mode))
        {
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_DIR_ALREADY_EXIST";
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
        if(!(flags & KIO::Overwrite))
        {
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_FILE_ALREADY_EXIST";
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
    }
    qCDebug(KIO_SMB_LOG) << "smbc_rename " << src.toSmbcUrl() << " " << dst.toSmbcUrl();
    retVal = smbc_rename(src.toSmbcUrl(), dst.toSmbcUrl());
    if( retVal < 0 ){
        errNum = errno;
    } else {
        errNum = 0;
    }

    if( retVal < 0 )
    {
      qCDebug(KIO_SMB_LOG) << "failed ";
      switch(errNum)
      {
        case ENOENT:
          errNum = cache_stat(src, &st);
          if( errNum != 0 )
          {
              if(errNum == EACCES)
	      {
	        qCDebug(KIO_SMB_LOG) << "KIO::ERR_ACCESS_DENIED";
                error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
              }
              else
              {
		qCDebug(KIO_SMB_LOG) << "KIO::ERR_DOES_NOT_EXIST";
                error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
              }
          }
          break;

        case EACCES:
        case EPERM:
          qCDebug(KIO_SMB_LOG) << "KIO::ERR_ACCESS_DENIED";
          error( KIO::ERR_ACCESS_DENIED, dst.toDisplayString() );
          break;

        default:
          qCDebug(KIO_SMB_LOG) << "KIO::ERR_CANNOT_RENAME";
          error( KIO::ERR_CANNOT_RENAME, src.toDisplayString() );

      }

      qCDebug(KIO_SMB_LOG) << "exit with error";
      return;
    }

    qCDebug(KIO_SMB_LOG) << "everything fine\n";
    finished();
}
