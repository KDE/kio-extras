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
//     a copy from http://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "kio_smb.h"
#include "kio_smb_internal.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include <kde_file.h>
#include <kconfiggroup.h>
#include <kdeversion.h>
#include <kio/ioslave_defaults.h>

//===========================================================================
void SMBSlave::copy(const KUrl& src, const KUrl& dst, int permissions, KIO::JobFlags flags)
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

void SMBSlave::smbCopy(const KUrl& ksrc, const KUrl& kdst, int permissions, KIO::JobFlags flags)
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

    kDebug(KIO_SMB) << "SMBSlave::copy with src = " << ksrc << "and dest = " << kdst;

    // setup urls
    src = ksrc;
    dst = kdst;

    // Obtain information about source
    errNum = cache_stat(src, &st );
    if( errNum != 0 )
    {
        if ( errNum == EACCES )
        {
            error( KIO::ERR_ACCESS_DENIED, src.prettyUrl());
        }
        else
        {
             error( KIO::ERR_DOES_NOT_EXIST, src.prettyUrl());
        }
        return;
    }
    if ( S_ISDIR( st.st_mode ) )
    {
        error( KIO::ERR_IS_DIRECTORY, src.prettyUrl() );
        return;
    }
    totalSize(st.st_size);

    // Check to se if the destination exists
    errNum = cache_stat(dst, &st);
    if( errNum == 0 )
    {
        if(S_ISDIR(st.st_mode))
        {
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.prettyUrl());
	    return;
        }
        if(!(flags & KIO::Overwrite))
        {
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.prettyUrl());
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
            error( KIO::ERR_ACCESS_DENIED, src.prettyUrl() );
        }
        else
        {
            error( KIO::ERR_DOES_NOT_EXIST, src.prettyUrl() );
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
            error(KIO::ERR_WRITE_ACCESS_DENIED, dst.prettyUrl());
        }
        else
        {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, dst.prettyUrl());
        }

        if(srcfd >= 0 )
        {
            smbc_close(srcfd);
        }
        return;
    }


    // Perform copy
    while(1)
    {
        n = smbc_read(srcfd, buf, MAX_XFER_BUF_SIZE );
        if(n > 0)
        {
            n = smbc_write(dstfd, buf, n);
            if(n == -1)
            {
	        kDebug(KIO_SMB) << "SMBSlave::copy copy now KIO::ERR_COULD_NOT_WRITE";
                error( KIO::ERR_COULD_NOT_WRITE, dst.prettyUrl());
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
            error( KIO::ERR_COULD_NOT_READ, src.prettyUrl());
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
            error( KIO::ERR_COULD_NOT_WRITE, dst.prettyUrl());
	    return;
        }
    }

    finished();
}

void SMBSlave::smbCopyGet(const KUrl& ksrc, const KUrl& kdst, int permissions, KIO::JobFlags flags)
{
    kDebug(KIO_SMB) << "src = " << ksrc << ", dest = " << kdst;

    // check if destination is ok ...
    const QString dstFile = kdst.toLocalFile();
    const QFileInfo dstInfo (dstFile);

    if(dstInfo.exists())  {
        if(dstInfo.isDir()) {
            error (ERR_IS_DIRECTORY, kdst.prettyUrl());
            return;
        }

        if(!(flags & KIO::Overwrite)) {
            error(ERR_FILE_ALREADY_EXIST, kdst.prettyUrl());
            return;
        }
    }

    bool bResume = false;
    const QFileInfo partInfo (dstFile + QLatin1String(".part"));
    const bool bPartExists = partInfo.exists();
    const bool bMarkPartial = config()->readEntry("MarkPartial", true);

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
#if KDE_IS_VERSION(4,11,80)
    if (!bResume) {
        QFile::Permissions perms;
        if (permissions == -1) {
            perms = QFile::ReadOwner | QFile::WriteOwner;
        } else {
            perms = KIO::convertPermissions(permissions | QFile::WriteOwner);
        }
        file.setPermissions(perms);
    }
#endif
    if (!file.open(mode)) {
        kDebug(KIO_SMB) << "could not write to" << dstFile;
        switch (file.error()) {
          case QFile::OpenError:
              if (bResume) {
                error (ERR_CANNOT_RESUME, kdst.prettyUrl());
              } else {
                error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.prettyUrl());
              }
              break;
          case QFile::PermissionsError:
              error(ERR_WRITE_ACCESS_DENIED, kdst.prettyUrl());
              break;
          default:
              error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.prettyUrl());
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
            error (KIO::ERR_ACCESS_DENIED, src.prettyUrl());
        } else {
            error (KIO::ERR_DOES_NOT_EXIST, src.prettyUrl());
        }
        return;
    }

    if (S_ISDIR( st.st_mode )) {
        error (KIO::ERR_IS_DIRECTORY, src.prettyUrl());
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
            kDebug(KIO_SMB) << "seeking to size" << partInfo.size();
            off_t offset = smbc_lseek(srcfd, partInfo.size(), SEEK_SET);
            if (offset == -1) {
                error(KIO::ERR_COULD_NOT_SEEK, src.prettyUrl());
                smbc_close(srcfd);
                return;
            } else {
                processed_size += offset;
            }
        }
    }

    if (srcfd < 0) {
        if(errNum == EACCES) {
            error( KIO::ERR_ACCESS_DENIED, src.prettyUrl() );
        } else {
            error( KIO::ERR_DOES_NOT_EXIST, src.prettyUrl() );
        }
        return;
    }

    // Perform the copy
    char buf[MAX_XFER_BUF_SIZE];
    bool isErr = false;

    while (1) {
        const ssize_t bytesRead = smbc_read(srcfd, buf, MAX_XFER_BUF_SIZE);
        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                error( KIO::ERR_COULD_NOT_READ, src.prettyUrl());
                isErr = true;
            }
            break;
        }

        const qint64 bytesWritten = file.write(buf, bytesRead);
        if (bytesWritten == -1) {
            kDebug(KIO_SMB) << "copy now KIO::ERR_COULD_NOT_WRITE";
            error( KIO::ERR_COULD_NOT_WRITE, kdst.prettyUrl());
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
            const int size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
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
            kDebug(KIO_SMB) << "failed to rename" << sPart << "to" << dstFile;
            error(ERR_CANNOT_RENAME_PARTIAL, sPart);
            return;
        }
    }

#if KDE_VERSION < KDE_MAKE_VERSION(4,11,80)
    if (permissions != -1 && !bResume) {
        if (KDE::chmod(dstFile, permissions) < 0) {
            kWarning(KIO_SMB) << "failed to set permission on" << dstFile;
        }
    }
#endif

    // Restore the mtime on the file.
    const QString mtimeStr = metaData("modified");
    kDebug(KIO_SMB) << "modified:" << mtimeStr;
    if (!mtimeStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dt.isValid()) {
            struct utimbuf utbuf;
            utbuf.actime = QFileInfo(file).lastRead().toTime_t(); // access time, unchanged
            utbuf.modtime = dt.toTime_t(); // modification time
            KDE::utime(dstFile, &utbuf);
        }
    }

    finished();
}

void SMBSlave::smbCopyPut(const KUrl& ksrc, const KUrl& kdst, int permissions, KIO::JobFlags flags)
{
    kDebug(KIO_SMB) << "src = " << ksrc << ", dest = " << kdst;

    QFile srcFile (ksrc.toLocalFile());
    const QFileInfo srcInfo (srcFile);

    if (srcInfo.exists()) {
        if (srcInfo.isDir()) {
            error(KIO::ERR_IS_DIRECTORY, ksrc.prettyUrl());
            return;
        }
    } else {
        error(KIO::ERR_DOES_NOT_EXIST, ksrc.prettyUrl());
        return;
    }

    if (!srcFile.open(QFile::ReadOnly)) {
        kDebug(KIO_SMB) << "could not read from" << ksrc;
        switch (srcFile.error()) {
          case QFile::PermissionsError:
              error(KIO::ERR_WRITE_ACCESS_DENIED, ksrc.prettyUrl());
              break;
          case QFile::OpenError:
          default:
              error(KIO::ERR_CANNOT_OPEN_FOR_READING, ksrc.prettyUrl());
              break;
        }
        return;
    }

    totalSize(static_cast<filesize_t>(srcInfo.size()));

    bool bResume = false;
    bool bPartExists = false;
    const bool bMarkPartial = config()->readEntry("MarkPartial", true);
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
            error( KIO::ERR_IS_DIRECTORY, dstOrigUrl.prettyUrl());
        } else {
            error( KIO::ERR_FILE_ALREADY_EXIST, dstOrigUrl.prettyUrl());
        }
        return;
    }

    KIO::filesize_t processed_size = 0;
    const SMBUrl dstUrl(bMarkPartial ? dstOrigUrl.partUrl() : dstOrigUrl);

    if (bResume) {
        // append if resuming
        kDebug(KIO_SMB) << "resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_RDWR, 0 );
        if (dstfd < 0) {
            errNum = errno;
        } else {
            const off_t offset = smbc_lseek(dstfd, 0, SEEK_END);
            if (offset == (off_t)-1) {
                error(KIO::ERR_COULD_NOT_SEEK, dstUrl.prettyUrl());
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

        kDebug(KIO_SMB) << "NO resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (dstfd < 0) {
            errNum = errno;
        }
    }

    if (dstfd < 0) {
        if (errNum == EACCES) {
            kDebug(KIO_SMB) << "access denied";
            error( KIO::ERR_WRITE_ACCESS_DENIED, dstUrl.prettyUrl());
        }
        else {
            kDebug(KIO_SMB) << "can not open for writing";
            error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dstUrl.prettyUrl());
        }
        return;
    }

    bool isErr = false;

    if (processed_size == 0 || srcFile.seek(processed_size)) {
        // Perform the copy
        char buf[MAX_XFER_BUF_SIZE];

        while (1) {
            const ssize_t bytesRead = srcFile.read(buf, MAX_XFER_BUF_SIZE);
            if (bytesRead <= 0) {
                if (bytesRead < 0) {
                    error(KIO::ERR_COULD_NOT_READ, ksrc.prettyUrl());
                    isErr = true;
                }
                break;
            }

            const qint64 bytesWritten = smbc_write(dstfd, buf, bytesRead);
            if (bytesWritten == -1) {
                error(KIO::ERR_COULD_NOT_WRITE, kdst.prettyUrl());
                isErr = true;
                break;
            }

            processed_size += bytesWritten;
            processedSize(processed_size);
        }
    } else {
        isErr = true;
        error(KIO::ERR_COULD_NOT_SEEK, ksrc.prettyUrl());
    }

    // FINISHED
    if (smbc_close(dstfd) < 0) {
        kDebug(KIO_SMB) << dstUrl << "could not write";
        error( KIO::ERR_COULD_NOT_WRITE, dstUrl.prettyUrl());
        return;
    }

    // Handle error condition.
    if (isErr) {
        if (bMarkPartial) {
            const int size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
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
            kDebug(KIO_SMB) << "failed to rename" << dstUrl << "to" << dstOrigUrl << "->" << strerror(errno);
            error(ERR_CANNOT_RENAME_PARTIAL, dstUrl.prettyUrl());
            return;
        }
    }

#ifdef HAVE_UTIME_H
    // set modification time
    const QString mtimeStr = metaData( "modified" );
    if (!mtimeStr.isEmpty() ) {
        QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
        if ( dt.isValid() ) {
            struct utimbuf utbuf;
            utbuf.actime = st.st_atime; // access time, unchanged
            utbuf.modtime = dt.toTime_t(); // modification time
            smbc_utime( dstUrl.toSmbcUrl(), &utbuf );
        }
    }
#endif

    // We have done our job => finish
    finished();
}

//===========================================================================
void SMBSlave::del( const KUrl &kurl, bool isfile)
{
    kDebug(KIO_SMB) << kurl;
    m_current_url = kurl;
    int errNum = 0;
    int retVal = 0;

    if(isfile)
    {
        // Delete file
        kDebug(KIO_SMB) << kurl;
        retVal = smbc_unlink(m_current_url.toSmbcUrl());
        if ( retVal < 0 ){
            errNum = errno;
        } else {
            errNum = 0;
        }
    }
    else
    {
        kDebug(KIO_SMB) << kurl;
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
void SMBSlave::mkdir( const KUrl &kurl, int permissions )
{
    kDebug(KIO_SMB) << kurl;
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
                error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.prettyUrl());
            }
            else
            {
                error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.prettyUrl());
            }
        }
        else
        {
            reportError(kurl, errNum);
        }
        kDebug(KIO_SMB) << "exit with error " << kurl;
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
void SMBSlave::rename( const KUrl& ksrc, const KUrl& kdest, KIO::JobFlags flags )
{

    SMBUrl      src;
    SMBUrl      dst;
    int         errNum = 0;
    int         retVal = 0;

    kDebug(KIO_SMB) << "old name = " << ksrc << ", new name = " << kdest;

    src = ksrc;
    dst = kdest;

    // Check to se if the destination exists

    kDebug(KIO_SMB) << "stat dst";
    errNum = cache_stat(dst, &st);
    if( errNum == 0 )
    {
        if(S_ISDIR(st.st_mode))
        {
            kDebug(KIO_SMB) << "KIO::ERR_DIR_ALREADY_EXIST";
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.prettyUrl());
            return;
        }
        if(!(flags & KIO::Overwrite))
        {
            kDebug(KIO_SMB) << "KIO::ERR_FILE_ALREADY_EXIST";
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.prettyUrl());
            return;
        }
    }
    kDebug(KIO_SMB ) << "smbc_rename " << src.toSmbcUrl() << " " << dst.toSmbcUrl();
    retVal = smbc_rename(src.toSmbcUrl(), dst.toSmbcUrl());
    if( retVal < 0 ){
        errNum = errno;
    } else {
        errNum = 0;
    }

    if( retVal < 0 )
    {
      kDebug(KIO_SMB ) << "failed ";
      switch(errNum)
      {
        case ENOENT:
          errNum = cache_stat(src, &st);
          if( errNum != 0 )
          {
              if(errNum == EACCES)
	      {
	        kDebug(KIO_SMB) << "KIO::ERR_ACCESS_DENIED";
                error(KIO::ERR_ACCESS_DENIED, src.prettyUrl());
              }
              else
              {
		kDebug(KIO_SMB) << "KIO::ERR_DOES_NOT_EXIST";
                error(KIO::ERR_DOES_NOT_EXIST, src.prettyUrl());
              }
          }
          break;

        case EACCES:
        case EPERM:
          kDebug(KIO_SMB) << "KIO::ERR_ACCESS_DENIED";
          error( KIO::ERR_ACCESS_DENIED, dst.prettyUrl() );
          break;

        default:
          kDebug(KIO_SMB) << "KIO::ERR_CANNOT_RENAME";
          error( KIO::ERR_CANNOT_RENAME, src.prettyUrl() );

      }

      kDebug(KIO_SMB) << "exit with error";
      return;
    }

    kDebug(KIO_SMB ) << "everything fine\n";
    finished();
}
