
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
// Copyright (c) 2018  Harald Sitter <sitter@kde.org>
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

#include <DNSSD/ServiceBrowser>
#include <DNSSD/RemoteService>
#include <KLocalizedString>
#include <KIO/Job>

#include <QEventLoop>

#include <pwd.h>
#include <grp.h>

#include <config-runtime.h>

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
    qCDebug(KIO_SMB) << "size " << (KIO::filesize_t)st->st_size;
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
         qCDebug(KIO_SMB) << "mode: "<< st.st_mode;
         warning(i18n("%1:\n"
                      "Unknown file type, neither directory or file.", url.toDisplayString()));
         return EINVAL;
      }

      udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, st.st_mode & S_IFMT);
      udsentry.fastInsert(KIO::UDSEntry::UDS_SIZE, st.st_size);

      QString str;
      uid_t uid = st.st_uid;
      struct passwd *user = getpwuid( uid );
      if ( user )
          str = user->pw_name;
      else
          str = QString::number( uid );
      udsentry.fastInsert(KIO::UDSEntry::UDS_USER, str);

      gid_t gid = st.st_gid;
      struct group *grp = getgrgid( gid );
      if ( grp )
          str = grp->gr_name;
      else
          str = QString::number( gid );
      udsentry.fastInsert(KIO::UDSEntry::UDS_GROUP, str);

      udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, st.st_mode & 07777);
      udsentry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, st.st_mtime);
      udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS_TIME, st.st_atime);
      // No, st_ctime is not UDS_CREATION_TIME...
   }

   return cacheStatErr;
}

//===========================================================================
void SMBSlave::stat( const QUrl& kurl )
{
    qCDebug(KIO_SMB) << kurl;
    // make a valid URL
    QUrl url = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (url != kurl)
    {
        qCDebug(KIO_SMB) << "redirection " << url;
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
                qCDebug(KIO_SMB) << "stat() error" << ret << url;
                reportError(url, ret);
                return;
            }
            break;
        }
    default:
        qCDebug(KIO_SMB) << "UNKNOWN " << url;
        finished();
        return;
    }

    statEntry(udsentry);
    finished();
}

//===========================================================================
// TODO: complete checking
QUrl SMBSlave::checkURL(const QUrl& kurl) const
{
    qCDebug(KIO_SMB) << "checkURL " << kurl;
    QString surl = kurl.url();
    //transform any links in the form smb:/ into smb://
    if (surl.startsWith(QLatin1String("smb:/"))) {
        if (surl.length() == 5) {
            return QUrl("smb://");
        }
        if (surl.at(5) != '/') {
            surl = "smb://" + surl.mid(5);
            qCDebug(KIO_SMB) << "checkURL return1 " << surl << " " << QUrl(surl);
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
        qCDebug(KIO_SMB) << "checkURL return2 " << url;
        return url;
    }

    //if there's a valid host, don't have an empty path
    QUrl url(kurl);

    if (url.path().isEmpty())
        url.setPath("/");

    qCDebug(KIO_SMB) << "checkURL return3 " << url;
    return url;
}

SMBSlave::SMBError SMBSlave::errnumToKioError(const SMBUrl &url, const int errNum)
{
    qCDebug(KIO_SMB) << "errNum" << errNum;

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
   qCDebug(KIO_SMB) << kurl;
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

   qCDebug(KIO_SMB) << "open " << m_current_url.toSmbcUrl() << " " << m_current_url.getType() << " " << dirfd;
   if(dirfd >= 0)
   {
       uint direntCount = 0;
       do {
           qCDebug(KIO_SMB) << "smbc_readdir ";
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

           qCDebug(KIO_SMB) << "dirp->name " <<  dirp->name  << " " << dirpName << " '" << comment << "'" << " " << dirp->smbc_type;

           udsentry.fastInsert( KIO::UDSEntry::UDS_NAME, udsName );

           // Mark all administrative shares, e.g ADMIN$, as hidden. #197903
           if (dirpName.endsWith(QLatin1Char('$'))) {
              //qCDebug(KIO_SMB) << dirpName << "marked as hidden";
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
                   qCDebug(KIO_SMB) << "list item " << u;
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
               qCDebug(KIO_SMB) << "SMBC_UNKNOWN :" << dirpName;
               // TODO: we don't handle SMBC_IPC_SHARE, SMBC_PRINTER_SHARE
               //       SMBC_LINK, SMBC_COMMS_SHARE
               //SlaveBase::error(ERR_INTERNAL, TEXT_UNSUPPORTED_FILE_TYPE);
               // continue;
           }
           udsentry.clear();
       } while (dirp); // checked already in the head

       listDNSSD(udsentry, url, direntCount);

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

void SMBSlave::listDNSSD(UDSEntry &udsentry, const QUrl &url, const uint direntCount)
{
    // Certain versions of KDNSSD suffer from signal races which can easily
    // deadlock the slave.
#ifndef HAVE_KDNSSD_WITH_SIGNAL_RACE_PROTECTION
    return;
#endif // HAVE_KDNSSD_WITH_SIGNAL_RACE_PROTECTION

    // This entire method act as fallback logic iff SMB discovery is not working
    // (for example when using a protocol version that doesn't have discovery).
    // As such we can return if entries were discovered or the URL is not '/'
    auto normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    if (direntCount > 0 || !normalizedUrl.path().isEmpty()) {
        return;
    }

    // Slaves have no event loop, start one for the poll.
    // KDNSSD has an internal timeout which may trigger if this takes too long
    // so in theory this should not ever be able to get stuck.
    // The eventloop runs until the discovery is finished. The finished slot
    // will quit it.
    QList<KDNSSD::RemoteService::Ptr> services;
    QEventLoop e;
    KDNSSD::ServiceBrowser browser(QStringLiteral("_smb._tcp"));
    connect(&browser, &KDNSSD::ServiceBrowser::serviceAdded,
            this, [&services](KDNSSD::RemoteService::Ptr service){
        qCDebug(KIO_SMB) << "DNSSD added:"
                         << service->serviceName()
                         << service->type()
                         << service->domain()
                         << service->hostName()
                         << service->port();
        // Manual contains check. We need to use the == of the underlying
        // objects, not the pointers. The same service may have >1
        // RemoteService* instances representing it, so the == impl of
        // RemoteService::Ptr is useless here.
        for (const auto &it : services) {
            if (*service == *it) {
                return;
            }
        }
        // Schedule resolution of hostname. We'll later call resolve
        // which will block until the resolution is done. This basically
        // gives us a head start.
        service->resolveAsync();
        services.append(service);
    });
    connect(&browser, &KDNSSD::ServiceBrowser::serviceRemoved,
            this, [&services](KDNSSD::RemoteService::Ptr service){
        qCDebug(KIO_SMB) << "DNSSD removed:"
                         << service->serviceName()
                         << service->type()
                         << service->domain()
                         << service->hostName()
                         << service->port();
        services.removeAll(service);
    });
    connect(&browser, &KDNSSD::ServiceBrowser::finished,
            this, [&]() {
        browser.disconnect(); // Stop sending anything, we'll exit here.
        // Resolution still requires an event loop. So, let the resolutions
        // finish and then quit the loop. Services that fail resolution
        // get dropped since we won't be able to access them properly.
        for (auto it = services.begin(); it != services.end(); ++it) {
            auto service = *it;
            if (!service->resolve()) {
                qCWarning(KIO_SMB) << "Failed to resolve DNSSD service"
                                   << service->serviceName();
                it = services.erase(it);
            }
        }
        e.quit();
    });
    browser.startBrowse();
    e.exec();

    for (const auto &service : services) {
        udsentry.fastInsert(KIO::UDSEntry::UDS_NAME, service->serviceName());

        udsentry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        udsentry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));

        // TODO: it may be better to resolve the host to an ip address. dnssd
        //   being able to find a service doesn't mean name resolution is
        //   properly set up for its domain. So, we may not be able to resolve
        //   this without help from avahi. OTOH KDNSSD doesn't have API for this
        //   and from a platform POV we should probably assume that if avahi
        //   is functional it is also set up as resolution provider.
        //   Given the plugin design on glibc's libnss however I am not sure
        //   that assumption will be true all the time. ~sitter, 2018
        QUrl u(QStringLiteral("smb://"));
        u.setHost(service->hostName());
        if (service->port() > 0 && service->port() != 445 /* default smb */) {
            u.setPort(service->port());
        }

        udsentry.fastInsert(KIO::UDSEntry::UDS_URL, u.url());
        udsentry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE,
                            QStringLiteral("application/x-smb-server"));

        listEntry(udsentry);
        udsentry.clear();
    }

    // NOTE: workgroups cannot be properly resolved because libsmbclient
    //   seems to lack the appropriate API to pull this data out of netbios.
    //   Netbios is also not working on IPv6 and its replacement (LLMNR)
    //   doesn't support the concept of workgroups.
}

void SMBSlave::fileSystemFreeSpace(const QUrl& url)
{
    qCDebug(KIO_SMB) << url;

    // Avoid crashing in smbc_fstatvfs below when
    // requesting free space for smb:// which doesn't
    // make sense to do to begin with
    if (url.host().isEmpty()) {
        error(KIO::ERR_CANNOT_STAT, url.url());
        return;
    }

    SMBUrl smbcUrl = url;
    int handle = smbc_opendir(smbcUrl.toSmbcUrl());
    if (handle < 0) {
       error(KIO::ERR_CANNOT_STAT, url.url());
       return;
    }

    struct statvfs dirStat;
    memset(&dirStat, 0, sizeof(struct statvfs));
    int err = smbc_fstatvfs(handle, &dirStat);
    smbc_closedir(handle);

    if (err < 0) {
       error(KIO::ERR_CANNOT_STAT, url.url());
       return;
    }

    KIO::filesize_t blockSize;
    if (dirStat.f_frsize != 0) {
       blockSize = dirStat.f_frsize;
    } else {
       blockSize = dirStat.f_bsize;
    }

    setMetaData("total", QString::number(blockSize * dirStat.f_blocks));
    setMetaData("available", QString::number(blockSize * dirStat.f_bavail));

    finished();
}

bool SMBSlave::workaroundEEXIST(const int errNum) const
{
    return (errNum == EEXIST) && m_enableEEXISTWorkaround;
}

