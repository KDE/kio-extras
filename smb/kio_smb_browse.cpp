
/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_browse.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              SMB browsing
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
// Copyright (c) 2018-2020  Harald Sitter <sitter@kde.org>
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

#include <DNSSD/ServiceBrowser>
#include <DNSSD/RemoteService>
#include <KLocalizedString>
#include <KIO/Job>

#include <QEventLoop>

#include <QTimer>

#include <pwd.h>
#include <grp.h>

#include <config-runtime.h>
#include "wsdiscoverer.h"
#include "dnssddiscoverer.h"

using namespace KIO;

int SMBSlave::cache_stat(const SMBUrl &url, struct stat* st )
{
    int cacheStatErr;
    int result = smbc_stat( url.toSmbcUrl(), st);
    if (result == 0){
        cacheStatErr = 0;
    } else {
        cacheStatErr = errno;
    }
    qCDebug(KIO_SMB_LOG) << "size " << (KIO::filesize_t)st->st_size;
    return cacheStatErr;
}

//---------------------------------------------------------------------------
int SMBSlave::browse_stat_path(const SMBUrl& _url, UDSEntry& udsentry)
{
   SMBUrl url = _url;

   int cacheStatErr = cache_stat(url, &st);
   if(cacheStatErr == 0)
   {
      if(!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
      {
         qCDebug(KIO_SMB_LOG) << "mode: "<< st.st_mode;
         warning(i18n("%1:\n"
                      "Unknown file type, neither directory or file.", url.toDisplayString()));
         return EINVAL;
      }

      if (!S_ISDIR(st.st_mode)) {
          // Awkwardly documented at
          //    https://www.samba.org/samba/docs/using_samba/ch08.html
          // libsmb_stat.c assigns special meaning to +x permissions
          // (obviously only on files, all dirs are +x so this hacky representation
          //  wouldn't work!):
          // - S_IXUSR = DOS archive: This file has been touched since the last DOS backup was performed on it.
          // - S_IXGRP = DOS system: This file has a specific purpose required by the operating system.
          // - S_IXOTH = DOS hidden: This file has been marked to be invisible to the user, unless the operating system is explicitly set to show it.
          // Only hiding has backing through KIO right now.
          if (st.st_mode & S_IXOTH) { // DOS hidden
              udsentry.fastInsert(KIO::UDSEntry::UDS_HIDDEN, true);
          }
      }

      // UID and GID **must** not be mapped. The values returned by libsmbclient are
      // simply the getuid/getgid of the process. They mean absolutely nothing.
      // Also see libsmb_stat.c.
      // Related: https://bugs.kde.org/show_bug.cgi?id=212801

      // POSIX Access mode must not be mapped either!
      // It's meaningless for smb shares and downright disadvantagous.
      // The mode attributes outside the ones used and document above are
      // useless. The only one actively set is readonlyness.
      //
      // BUT the READONLY attribute does nothing on NT systems:
      // https://support.microsoft.com/en-us/help/326549/you-cannot-view-or-change-the-read-only-or-the-system-attributes-of-fo
      // The Read-only and System attributes is only used by Windows Explorer to determine
      // whether the folder is a special folder, such as a system folder that has its view
      // customized by Windows (for example, My Documents, Favorites, Fonts, Downloaded Program Files),
      // or a folder that you customized by using the Customize tab of the folder's Properties dialog box.
      //
      // As such respecting it on a KIO level is actually wrong as it doesn't indicate actual
      // readonlyness since the 90s and causes us to show readonly UI states when in fact
      // the directory is perfectly writable.
      // https://bugs.kde.org/show_bug.cgi?id=414482
      //
      // Should we ever want to parse desktop.ini like we do .directory we'd only want to when a
      // dir is readonly as per the above microsoft support article.
      // Also see:
      // https://docs.microsoft.com/en-us/windows/win32/shell/how-to-customize-folders-with-desktop-ini
      udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, -1);

      udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, st.st_mode & S_IFMT);
      udsentry.fastInsert(KIO::UDSEntry::UDS_SIZE, st.st_size);
      udsentry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, st.st_mtime);
      udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS_TIME, st.st_atime);
      // No, st_ctime is not UDS_CREATION_TIME...
   }

   return cacheStatErr;
}

//===========================================================================
void SMBSlave::stat( const QUrl& kurl )
{
    qCDebug(KIO_SMB_LOG) << kurl;
    // make a valid URL
    QUrl url = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (url != kurl)
    {
        qCDebug(KIO_SMB_LOG) << "redirection " << url;
        redirection(url);
        finished();
        return;
    }

    m_current_url = url;

    UDSEntry    udsentry;
    // Set name
    udsentry.fastInsert( KIO::UDSEntry::UDS_NAME, kurl.fileName() );

    switch(m_current_url.getType())
    {
    case SMBURLTYPE_UNKNOWN:
        error(ERR_MALFORMED_URL, url.toDisplayString());
        return;

    case SMBURLTYPE_ENTIRE_NETWORK:
    case SMBURLTYPE_WORKGROUP_OR_SERVER:
        udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        break;

    case SMBURLTYPE_SHARE_OR_PATH:
        {
            int ret = browse_stat_path(m_current_url, udsentry);

            if (ret == EPERM || ret == EACCES || workaroundEEXIST(ret))
            {
                SMBUrl smbUrl(url);

                const int passwordError = checkPassword(smbUrl);
                if (passwordError == KJob::NoError)
                {
                    redirection(smbUrl);
                    finished();
                }
                else if (passwordError == KIO::ERR_USER_CANCELED)
                {
                    reportError(url, ret);
                }
                else
                {
                    error(passwordError, url.toString());
                }

                return;
            }
            else if (ret != 0)
            {
                qCDebug(KIO_SMB_LOG) << "stat() error" << ret << url;
                reportError(url, ret);
                return;
            }
            break;
        }
    default:
        qCDebug(KIO_SMB_LOG) << "UNKNOWN " << url;
        finished();
        return;
    }

    statEntry(udsentry);
    finished();
}

//===========================================================================
// TODO: complete checking <-- what does that even mean?
// TODO: why is this not part of SMBUrl or at the very least URL validation should
//    be 100% shared between this and SMBUrl. Notably SMBUrl has code that looks
//    to do a similar thing but is much less complete.
QUrl SMBSlave::checkURL(const QUrl &kurl_) const
{
    qCDebug(KIO_SMB_LOG) << "checkURL " << kurl_;

    QUrl kurl(kurl_);
    // We treat cifs as an alias but need to translate it to smb.
    // https://bugs.kde.org/show_bug.cgi?id=327295
    // It's not IANA registered and also libsmbc internally expects
    // smb URIs so we do very broadly coerce cifs to smb.
    // Also see SMBUrl.
    if (kurl.scheme() == "cifs") {
        kurl.setScheme("smb");
    }

    QString surl = kurl.url();
    //transform any links in the form smb:/ into smb://
    if (surl.startsWith(QLatin1String("smb:/"))) {
        if (surl.length() == 5) {
            return QUrl("smb://");
        }
        if (surl.at(5) != '/') {
            surl = "smb://" + surl.mid(5);
            qCDebug(KIO_SMB_LOG) << "checkURL return1 " << surl << " " << QUrl(surl);
            return QUrl(surl);
        }
    }
    if (surl == QLatin1String("smb://")) {
        return kurl; //unchanged
    }

    // smb:// normally have no userinfo
    // we must redirect ourself to remove the username and password
    if (surl.contains('@') && !surl.contains("smb://")) {
        QUrl url(kurl);
        url.setPath('/'+kurl.url().right( kurl.url().length()-kurl.url().indexOf('@') -1));
        QString userinfo = kurl.url().mid(5, kurl.url().indexOf('@')-5);
        if(userinfo.contains(':'))  {
            url.setUserName(userinfo.left(userinfo.indexOf(':')));
            url.setPassword(userinfo.right(userinfo.length()-userinfo.indexOf(':')-1));
        } else {
            url.setUserName(userinfo);
        }
        qCDebug(KIO_SMB_LOG) << "checkURL return2 " << url;
        return url;
    }

    //if there's a valid host, don't have an empty path
    QUrl url(kurl);

    if (url.path().isEmpty())
        url.setPath("/");

    qCDebug(KIO_SMB_LOG) << "checkURL return3 " << url;
    return url;
}

SMBSlave::SMBError SMBSlave::errnumToKioError(const SMBUrl &url, const int errNum)
{
    qCDebug(KIO_SMB_LOG) << "errNum" << errNum;

    switch(errNum)
    {
    case ENOENT:
        if (url.getType() == SMBURLTYPE_ENTIRE_NETWORK)
            return SMBError{ ERR_SLAVE_DEFINED, i18n("Unable to find any workgroups in your local network. This might be caused by an enabled firewall.") };
        else
            return SMBError{ ERR_DOES_NOT_EXIST, url.toDisplayString() };
#ifdef ENOMEDIUM
    case ENOMEDIUM:
        return SMBError{ ERR_SLAVE_DEFINED, i18n("No media in device for %1", url.toDisplayString()) };
#endif
#ifdef EHOSTDOWN
    case EHOSTDOWN:
#endif
    case ECONNREFUSED:
        return SMBError{ ERR_SLAVE_DEFINED, i18n("Could not connect to host for %1", url.toDisplayString()) };
        break;
    case ENOTDIR:
        return SMBError{ ERR_CANNOT_ENTER_DIRECTORY, url.toDisplayString() };
    case EFAULT:
    case EINVAL:
        return SMBError{ ERR_DOES_NOT_EXIST, url.toDisplayString() };
    case EPERM:
    case EACCES:
        return SMBError{ ERR_ACCESS_DENIED, url.toDisplayString() };
    case EIO:
    case ENETUNREACH:
        if ( url.getType() == SMBURLTYPE_ENTIRE_NETWORK || url.getType() == SMBURLTYPE_WORKGROUP_OR_SERVER )
            return SMBError{ ERR_SLAVE_DEFINED, i18n("Error while connecting to server responsible for %1", url.toDisplayString()) };
        else
            return SMBError{ ERR_CONNECTION_BROKEN, url.toDisplayString() };
    case ENOMEM:
        return SMBError{ ERR_OUT_OF_MEMORY, url.toDisplayString() };
    case ENODEV:
        return SMBError{ ERR_SLAVE_DEFINED, i18n("Share could not be found on given server") };
    case EBADF:
        return SMBError{ ERR_INTERNAL, i18n("Bad file descriptor") };
    case ETIMEDOUT:
        return SMBError{ ERR_SERVER_TIMEOUT, url.host() };
    case ENOTEMPTY:
        return SMBError{ ERR_CANNOT_RMDIR, url.toDisplayString() };
#ifdef ENOTUNIQ
    case ENOTUNIQ:
        return SMBError{ ERR_SLAVE_DEFINED, i18n("The given name could not be resolved to a unique server. "
                                                 "Make sure your network is setup without any name conflicts "
                                                 "between names used by Windows and by UNIX name resolution." ) };
#endif
    case ECONNABORTED:
        return SMBError{ ERR_CONNECTION_BROKEN, url.host() };
    case EHOSTUNREACH:
        return SMBError{ ERR_CANNOT_CONNECT,
                    i18nc("@info:status smb failed to reach the server (e.g. server offline or network failure). %1 is an ip address or hostname",
                          "%1: Host unreachable",
                          url.host()) };
    case 0: // success
      return SMBError{ ERR_INTERNAL, i18n("libsmbclient reported an error, but did not specify "
                                          "what the problem is. This might indicate a severe problem "
                                          "with your network - but also might indicate a problem with "
                                          "libsmbclient.\n"
                                          "If you want to help us, please provide a tcpdump of the "
                                          "network interface while you try to browse (be aware that "
                                          "it might contain private data, so do not post it if you are "
                                          "unsure about that - you can send it privately to the developers "
                                          "if they ask for it)") };
    default:
        return SMBError{ ERR_INTERNAL, i18n("Unknown error condition in stat: %1", QString::fromLocal8Bit( strerror(errNum))) };
    }
}

void SMBSlave::reportError(const SMBUrl& url, const int errNum)
{
    const SMBError smbErr = errnumToKioError(url, errNum);

    error(smbErr.kioErrorId, smbErr.errorString);
}

void SMBSlave::reportWarning(const SMBUrl& url, const int errNum)
{
    const SMBError smbErr = errnumToKioError(url, errNum);
    const QString errorString = buildErrorString(smbErr.kioErrorId, smbErr.errorString);

    warning(xi18n("Error occurred while trying to access %1<nl/>%2", url.url(), errorString));
}

//===========================================================================
void SMBSlave::listDir( const QUrl& kurl )
{
   qCDebug(KIO_SMB_LOG) << kurl;
   int errNum = 0;

   // check (correct) URL
   QUrl url = checkURL(kurl);
   // if URL is not valid we have to redirect to correct URL
   if (url != kurl)
   {
      redirection(url);
      finished();
      return;
   }

   m_current_url = kurl;

   int                 dirfd;
   struct smbc_dirent  *dirp = nullptr;
   UDSEntry    udsentry;
   bool dir_is_root = true;

   dirfd = smbc_opendir( m_current_url.toSmbcUrl() );
   if (dirfd > 0){
      errNum = 0;
   } else {
      errNum = errno;
   }

   qCDebug(KIO_SMB_LOG) << "open " << m_current_url.toSmbcUrl() << " " << m_current_url.getType() << " " << dirfd;
   if(dirfd >= 0)
   {
       uint direntCount = 0;
       do {
           qCDebug(KIO_SMB_LOG) << "smbc_readdir ";
           dirp = smbc_readdir(dirfd);
           if(dirp == nullptr)
               break;

           ++direntCount;

           // Set name
           QString udsName;
           const QString dirpName = QString::fromUtf8( dirp->name );
           // We cannot trust dirp->commentlen has it might be with or without the NUL character
           // See KDE bug #111430 and Samba bug #3030
           const QString comment = QString::fromUtf8( dirp->comment );
           if ( dirp->smbc_type == SMBC_SERVER || dirp->smbc_type == SMBC_WORKGROUP ) {
               udsName = dirpName.toLower();
               udsName[0] = dirpName.at( 0 ).toUpper();
               if ( !comment.isEmpty() && dirp->smbc_type == SMBC_SERVER )
                   udsName += " (" + comment + ')';
           } else
               udsName = dirpName;

           qCDebug(KIO_SMB_LOG) << "dirp->name " <<  dirp->name  << " " << dirpName << " '" << comment << "'" << " " << dirp->smbc_type;

           udsentry.fastInsert( KIO::UDSEntry::UDS_NAME, udsName );
           udsentry.fastInsert(KIO::UDSEntry::UDS_COMMENT, QString::fromUtf8(dirp->comment));

           // Mark all administrative shares, e.g ADMIN$, as hidden. #197903
           if (dirpName.endsWith(QLatin1Char('$'))) {
              //qCDebug(KIO_SMB_LOG) << dirpName << "marked as hidden";
              udsentry.fastInsert(KIO::UDSEntry::UDS_HIDDEN, 1);
           }

           if (udsName == ".")
           {
               // Skip the "." entry
               // Mind the way m_current_url is handled in the loop
           }
           else if (udsName == "..")
           {
               dir_is_root = false;
               // fprintf(stderr,"----------- hide: -%s-\n",dirp->name);
               // do nothing and hide the hidden shares
           }
           else if (dirp->smbc_type == SMBC_FILE ||
                    dirp->smbc_type == SMBC_DIR)
           {
               // Set stat information
               m_current_url.addPath(dirpName);
               const int statErr = browse_stat_path(m_current_url, udsentry);
               if (statErr)
               {
                   if (statErr == ENOENT || statErr == ENOTDIR)
                   {
                       reportWarning(m_current_url, statErr);
                   }
               }
               else
               {
                   // Call base class to list entry
                   listEntry(udsentry);
               }
               m_current_url.cd("..");
           }
           else if(dirp->smbc_type == SMBC_SERVER ||
                   dirp->smbc_type == SMBC_FILE_SHARE)
           {
               // Set type
               udsentry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );


               if (dirp->smbc_type == SMBC_SERVER) {
                   udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));

                   // QString workgroup = m_current_url.host().toUpper();
                   QUrl u("smb://");
                   u.setHost(dirpName);

                   // when libsmbclient knows
                   // u = QString("smb://%1?WORKGROUP=%2").arg(dirpName).arg(workgroup.toUpper());
                   qCDebug(KIO_SMB_LOG) << "list item " << u;
                   udsentry.fastInsert(KIO::UDSEntry::UDS_URL, u.url());

                   udsentry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("application/x-smb-server"));
               } else
                   udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));


               // Call base class to list entry
               listEntry(udsentry);
           }
           else if(dirp->smbc_type == SMBC_WORKGROUP)
           {
               // Set type
               udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);

               // Set permissions
               udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH));

               udsentry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("application/x-smb-workgroup"));

               // QString workgroup = m_current_url.host().toUpper();
               QUrl u("smb://");
               u.setHost(dirpName);
               udsentry.fastInsert(KIO::UDSEntry::UDS_URL, u.url());

               // Call base class to list entry
               listEntry(udsentry);
           }
           else
           {
               qCDebug(KIO_SMB_LOG) << "SMBC_UNKNOWN :" << dirpName;
               // TODO: we don't handle SMBC_IPC_SHARE, SMBC_PRINTER_SHARE
               //       SMBC_LINK, SMBC_COMMS_SHARE
               //SlaveBase::error(ERR_INTERNAL, TEXT_UNSUPPORTED_FILE_TYPE);
               // continue;
           }
           udsentry.clear();
       } while (dirp); // checked already in the head

       // Run service discovery if the path is root. This augments
       // "native" results from libsmbclient.
       auto normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
       if (normalizedUrl.path().isEmpty()) {
           qCDebug(KIO_SMB_LOG) << "Trying modern discovery (dnssd/wsdiscovery)";

           QEventLoop e;

           UDSEntryList list;

           const auto flushEntries = [this, &list]() {
               if (list.isEmpty()) {
                   return;
               }
               listEntries(list);
               list.clear();
           };

           const auto quitLoop = [&e, &flushEntries]() {
               flushEntries();
               e.quit();
           };

           // Since slavebase has no eventloop it wont publish results
           // on a timer, since we do not know how long our discovery
           // will take this is super meh because we may appear
           // stuck for a while. Implement our own listing system
           // based on QTimer to mitigate.
           QTimer sendTimer;
           sendTimer.setInterval(300);
           connect(&sendTimer, &QTimer::timeout, this, flushEntries);
           sendTimer.start();

           DNSSDDiscoverer d;
           WSDiscoverer w;

           const QList<Discoverer *> discoverers {&d, &w};

           auto appendDiscovery = [&](Discovery::Ptr discovery) {
               list.append(discovery->toEntry());
           };

           auto maybeFinished = [&] { // finishes if all discoveries finished
               bool allFinished = true;
               for (auto discoverer : discoverers) {
                   allFinished = allFinished && discoverer->isFinished();
               }
               if (allFinished) {
                   quitLoop();
               }
           };

           connect(&d, &DNSSDDiscoverer::newDiscovery, this, appendDiscovery);
           connect(&w, &WSDiscoverer::newDiscovery, this, appendDiscovery);

           connect(&d, &DNSSDDiscoverer::finished, this, maybeFinished);
           connect(&w, &WSDiscoverer::finished, this, maybeFinished);

           d.start();
           w.start();

           QTimer::singleShot(16000, &e, quitLoop); // max execution time!
           e.exec();

           qCDebug(KIO_SMB_LOG) << "Modern discovery finished.";
       }

       if (dir_is_root) {
           udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
           udsentry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
           udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH));
       }
       else
       {
           udsentry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
           const int statErr = browse_stat_path(m_current_url, udsentry);
           if (statErr)
           {
               if (statErr == ENOENT || statErr == ENOTDIR)
               {
                   reportWarning(m_current_url, statErr);
               }
               // Create a default UDSEntry if we could not stat the actual directory
               udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
               udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
           }
       }
       listEntry(udsentry);
       udsentry.clear();

       // clean up
       smbc_closedir(dirfd);
   }
   else
   {
       if (errNum == EPERM || errNum == EACCES || workaroundEEXIST(errNum)) {
           const int passwordError = checkPassword(m_current_url);
           if (passwordError == KJob::NoError) {
               redirection( m_current_url );
               finished();
           } else if (passwordError == KIO::ERR_USER_CANCELED) {
               reportError(m_current_url, errNum);
           } else {
               error(passwordError, m_current_url.toString());
           }

           return;
       }

       reportError(m_current_url, errNum);
       return;
   }

   finished();
}

void SMBSlave::fileSystemFreeSpace(const QUrl& url)
{
    qCDebug(KIO_SMB_LOG) << url;

    // Avoid crashing in smbc_fstatvfs below when
    // requesting free space for smb:// which doesn't
    // make sense to do to begin with
    if (url.host().isEmpty()) {
        error(KIO::ERR_CANNOT_STAT, url.url());
        return;
    }

    SMBUrl smbcUrl = url;

    struct statvfs dirStat;
    memset(&dirStat, 0, sizeof(struct statvfs));
    const int err = smbc_statvfs(smbcUrl.toSmbcUrl().data(), &dirStat);
    if (err < 0) {
       error(KIO::ERR_CANNOT_STAT, url.url());
       return;
    }

    // libsmb_stat.c has very awkward conditional branching that results
    // in data meaning different things based on context:
    // A samba host with unix extensions has f_frsize==0 and the f_bsize is
    // the actual block size. Any other server (such as windows) has a non-zero
    // f_frsize denoting the amount of sectors in a block and the f_bsize is
    // the amount of bytes in a sector. As such frsize*bsize is the actual
    // block size.
    // This was also broken in different ways throughout history, so depending
    // on the specific libsmbc versions the milage will vary. 4.7 to 4.11 are
    // at least behaving as described though.
    // https://bugs.kde.org/show_bug.cgi?id=298801
    const auto frames = (dirStat.f_frsize == 0) ? 1 : dirStat.f_frsize;
    const auto blockSize =  dirStat.f_bsize * frames;
    // Further more on older versions of samba f_bavail may not be set...
    const auto total = blockSize * dirStat.f_blocks;
    const auto available = blockSize * ((dirStat.f_bavail != 0) ? dirStat.f_bavail : dirStat.f_bfree);

    setMetaData("total", QString::number(total));
    setMetaData("available", QString::number(available));

    finished();
}

bool SMBSlave::workaroundEEXIST(const int errNum) const
{
    return (errNum == EEXIST) && m_enableEEXISTWorkaround;
}

