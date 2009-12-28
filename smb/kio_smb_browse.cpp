
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

#include <pwd.h>
#include <grp.h>


#include <kglobal.h>

#include "kio_smb.h"
#include "kio_smb_internal.h"

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
    kDebug(KIO_SMB) << "size " << (KIO::filesize_t)st->st_size;
    return cacheStatErr;
}

//---------------------------------------------------------------------------
bool SMBSlave::browse_stat_path(const SMBUrl& _url, UDSEntry& udsentry, bool ignore_errors)
  // Returns: true on success, false on failure
{
   SMBUrl url = _url;

   int cacheStatErr = cache_stat(url, &st);
   if(cacheStatErr == 0)
   {
      if(!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
      {
         kDebug(KIO_SMB)<<"SMBSlave::browse_stat_path mode: "<<st.st_mode;
         warning(i18n("%1:\n"
                      "Unknown file type, neither directory or file.", url.prettyUrl()));
         return false;
      }

      udsentry.insert(KIO::UDSEntry::UDS_FILE_TYPE, st.st_mode & S_IFMT);
      udsentry.insert(KIO::UDSEntry::UDS_SIZE, st.st_size);

      QString str;
      uid_t uid = st.st_uid;
      struct passwd *user = getpwuid( uid );
      if ( user )
          str = user->pw_name;
      else
          str = QString::number( uid );
      udsentry.insert(KIO::UDSEntry::UDS_USER, str);

      gid_t gid = st.st_gid;
      struct group *grp = getgrgid( gid );
      if ( grp )
          str = grp->gr_name;
      else
          str = QString::number( gid );
      udsentry.insert(KIO::UDSEntry::UDS_GROUP, str);

      udsentry.insert(KIO::UDSEntry::UDS_ACCESS, st.st_mode & 07777);
      udsentry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, st.st_mtime);
      udsentry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, st.st_atime);
      // No, st_ctime is not UDS_CREATION_TIME...
   }
   else
   {
       if (!ignore_errors) {
           if (cacheStatErr == EPERM || cacheStatErr == EACCES)
               if (checkPassword(url)) {
                   redirection( url );
                   return false;
               }

           reportError(url, cacheStatErr);
       } else if (cacheStatErr == ENOENT || cacheStatErr == ENOTDIR) {
           warning(i18n("File does not exist: %1", url.url()));
       }
       kDebug(KIO_SMB) << "SMBSlave::browse_stat_path ERROR!!";
       return false;
   }

   return true;
}

//===========================================================================
void SMBSlave::stat( const KUrl& kurl )
{
    kDebug(KIO_SMB) << "SMBSlave::stat on "<< kurl;
    // make a valid URL
    KUrl url = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (url != kurl)
    {
        kDebug() << "redirection " << url;
        redirection(url);
        finished();
        return;
    }

    m_current_url = url;

    UDSEntry    udsentry;
    // Set name
    udsentry.insert( KIO::UDSEntry::UDS_NAME, kurl.fileName() );

    switch(m_current_url.getType())
    {
    case SMBURLTYPE_UNKNOWN:
        error(ERR_MALFORMED_URL,m_current_url.prettyUrl());
        finished();
        return;

    case SMBURLTYPE_ENTIRE_NETWORK:
    case SMBURLTYPE_WORKGROUP_OR_SERVER:
        udsentry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        break;

    case SMBURLTYPE_SHARE_OR_PATH:
        if (browse_stat_path(m_current_url, udsentry, false))
            break;
        else {
            kDebug(KIO_SMB) << "SMBSlave::stat ERROR!!";
            finished();
            return;
        }
    default:
        kDebug(KIO_SMB) << "SMBSlave::stat UNKNOWN " << url;
        finished();
        return;
    }

    statEntry(udsentry);
    finished();
}

//===========================================================================
// TODO: complete checking
KUrl SMBSlave::checkURL(const KUrl& kurl) const
{
    kDebug(KIO_SMB) << "checkURL " << kurl;
    QString surl = kurl.url();
    if (surl.startsWith(QLatin1String("smb:/"))) {
        if (surl.length() == 5) // just the above
            return kurl; // unchanged

        if (surl.at(5) != '/') {
            surl = "smb://" + surl.mid(5);
            kDebug(KIO_SMB) << "checkURL return1 " << surl << " " << KUrl(surl);
            return KUrl(surl);
        }
    }

    // smb:/ normaly have no userinfo
    // we must redirect ourself to remove the username and password
    if (surl.contains('@') && !surl.contains("smb://")) {
        KUrl url(kurl);
        url.setPath('/'+kurl.url().right( kurl.url().length()-kurl.url().indexOf('@') -1));
        QString userinfo = kurl.url().mid(5, kurl.url().indexOf('@')-5);
        if(userinfo.contains(':'))  {
            url.setUser(userinfo.left(userinfo.indexOf(':')));
            url.setPass(userinfo.right(userinfo.length()-userinfo.indexOf(':')-1));
        } else {
            url.setUser(userinfo);
        }
        kDebug(KIO_SMB) << "checkURL return2 " << url;
        return url;
    }

    // no emtpy path
    KUrl url(kurl);

    if (url.path().isEmpty())
        url.setPath("/");

    kDebug(KIO_SMB) << "checkURL return3 " << url;
    return url;
}

void SMBSlave::reportError(const SMBUrl &url, const int &errNum)
{
    kDebug(KIO_SMB) << "errNum" << errNum;

    switch(errNum)
    {
    case ENOENT:
        if (url.getType() == SMBURLTYPE_ENTIRE_NETWORK)
            error( ERR_SLAVE_DEFINED, i18n("Unable to find any workgroups in your local network. This might be caused by an enabled firewall."));
        else
            error( ERR_DOES_NOT_EXIST, url.prettyUrl());
        break;
#ifdef ENOMEDIUM
    case ENOMEDIUM:
        error( ERR_SLAVE_DEFINED,
               i18n( "No media in device for %1", url.prettyUrl() ) );
        break;
#endif
#ifdef EHOSTDOWN
    case EHOSTDOWN:
#endif
    case ECONNREFUSED:
        error(  ERR_SLAVE_DEFINED,
                i18n( "Could not connect to host for %1", url.prettyUrl() ) );
        break;
    case ENOTDIR:
        error( ERR_CANNOT_ENTER_DIRECTORY, url.prettyUrl());
        break;
    case EFAULT:
    case EINVAL:
        error( ERR_DOES_NOT_EXIST, url.prettyUrl());
        break;
    case EPERM:
    case EACCES:
        error( ERR_ACCESS_DENIED, url.prettyUrl() );
        break;
    case EIO:
    case ENETUNREACH:
        if ( url.getType() == SMBURLTYPE_ENTIRE_NETWORK || url.getType() == SMBURLTYPE_WORKGROUP_OR_SERVER )
            error( ERR_SLAVE_DEFINED, i18n( "Error while connecting to server responsible for %1", url.prettyUrl() ) );
        else
            error( ERR_CONNECTION_BROKEN, url.prettyUrl());
        break;
    case ENOMEM:
        error( ERR_OUT_OF_MEMORY, url.prettyUrl() );
        break;
    case ENODEV:
        error( ERR_SLAVE_DEFINED, i18n("Share could not be found on given server"));
        break;
    case EBADF:
        error( ERR_INTERNAL, i18n("BAD File descriptor"));
        break;
    case ETIMEDOUT:
        error( ERR_SERVER_TIMEOUT, url.host() );
        break;
#ifdef ENOTUNIQ
    case ENOTUNIQ:
        error( ERR_SLAVE_DEFINED, i18n( "The given name could not be resolved to a unique server. "
                                        "Make sure your network is setup without any name conflicts "
                                        "between names used by Windows and by UNIX name resolution." ) );
        break;
#endif
    case 0: // success
	  error( ERR_INTERNAL, i18n("libsmbclient reported an error, but did not specify "
								"what the problem is. This might indicate a severe problem "
								"with your network - but also might indicate a problem with "
								"libsmbclient.\n"
								"If you want to help us, please provide a tcpdump of the "
								"network interface while you try to browse (be aware that "
								"it might contain private data, so do not post it if you are "
								"unsure about that - you can send it privately to the developers "
								"if they ask for it)") );
	  break;
    default:
        error( ERR_INTERNAL, i18n("Unknown error condition in stat: %1", QString::fromLocal8Bit( strerror(errNum))) );
    }
}

//===========================================================================
void SMBSlave::listDir( const KUrl& kurl )
{
   kDebug(KIO_SMB) << "SMBSlave::listDir on " << kurl;
   int errNum = 0;

   // check (correct) URL
   KUrl url = checkURL(kurl);
   // if URL is not valid we have to redirect to correct URL
   if (url != kurl)
   {
      redirection(url);
      finished();
      return;
   }

   m_current_url = kurl;

   int                 dirfd;
   struct smbc_dirent  *dirp = NULL;
   UDSEntry    udsentry;

   dirfd = smbc_opendir( m_current_url.toSmbcUrl() );
   if (dirfd > 0){
      errNum = 0;
   } else {
      errNum = errno;
   }

   kDebug(KIO_SMB) << "SMBSlave::listDir open " << m_current_url.toSmbcUrl() << " " << m_current_url.getType() << " " << dirfd;
   if(dirfd >= 0)
   {
       do {
           kDebug(KIO_SMB) << "smbc_readdir ";
           dirp = smbc_readdir(dirfd);
           if(dirp == 0)
               break;

           // Set name
           QString udsName;
           QString dirpName = QString::fromUtf8( dirp->name );
           // We cannot trust dirp->commentlen has it might be with or without the NUL character
           // See KDE bug #111430 and Samba bug #3030
           QString comment = QString::fromUtf8( dirp->comment );
           if ( dirp->smbc_type == SMBC_SERVER || dirp->smbc_type == SMBC_WORKGROUP ) {
               udsName = dirpName.toLower();
               udsName[0] = dirpName.at( 0 ).toUpper();
               if ( !comment.isEmpty() && dirp->smbc_type == SMBC_SERVER )
                   udsName += " (" + comment + ')';
           } else
               udsName = dirpName;

           kDebug(KIO_SMB) << "dirp->name " <<  dirp->name  << " " << dirpName << " '" << comment << "'" << " " << dirp->smbc_type;

           udsentry.insert( KIO::UDSEntry::UDS_NAME, udsName );

           if (udsName.toUpper()=="IPC$" || udsName=="." || udsName == ".." ||
               udsName.toUpper() == "ADMIN$" || udsName.toLower() == "printer$" || udsName.toLower() == "print$" )
           {
//            fprintf(stderr,"----------- hide: -%s-\n",dirp->name);
               // do nothing and hide the hidden shares
           }
           else if(dirp->smbc_type == SMBC_FILE)
           {
               // Set stat information
               m_current_url.addPath(dirpName);
               browse_stat_path(m_current_url, udsentry, true);
               m_current_url.cd("..");

               // Call base class to list entry
               listEntry(udsentry, false);
           }
           else if(dirp->smbc_type == SMBC_DIR)
           {
               m_current_url.addPath(dirpName);
               browse_stat_path(m_current_url, udsentry, true);
               m_current_url.cd("..");

               // Call base class to list entry
               listEntry(udsentry, false);
           }
           else if(dirp->smbc_type == SMBC_SERVER ||
                   dirp->smbc_type == SMBC_FILE_SHARE)
           {
               // Set type
               udsentry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );

               // Set permissions
               udsentry.insert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH));

               if (dirp->smbc_type == SMBC_SERVER) {
                   // QString workgroup = m_current_url.host().toUpper();
                   KUrl u("smb:/");
                   u.setHost(dirpName);

                   // when libsmbclient knows
                   // u = QString("smb://%1?WORKGROUP=%2").arg(dirpName).arg(workgroup.toUpper());
                   kDebug(KIO_SMB) << "list item " << u;
                   udsentry.insert(KIO::UDSEntry::UDS_URL, u.url());

                   udsentry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("application/x-smb-server"));
               }

               // Call base class to list entry
               listEntry(udsentry, false);
            }
           else if(dirp->smbc_type == SMBC_WORKGROUP)
           {
               // Set type
               udsentry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);

               // Set permissions
               udsentry.insert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH));

               udsentry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("application/x-smb-workgroup"));

               // QString workgroup = m_current_url.host().toUpper();
               KUrl u("smb:/");
               u.setHost(dirpName);
               udsentry.insert(KIO::UDSEntry::UDS_URL, u.url());

               // Call base class to list entry
               listEntry(udsentry, false);
           }
           else
           {
               kDebug(KIO_SMB) << "SMBSlave::listDir SMBC_UNKNOWN :" << dirpName;
               // TODO: we don't handle SMBC_IPC_SHARE, SMBC_PRINTER_SHARE
               //       SMBC_LINK, SMBC_COMMS_SHARE
               //SlaveBase::error(ERR_INTERNAL, TEXT_UNSUPPORTED_FILE_TYPE);
               // continue;
           }
           udsentry.clear();
       } while (dirp); // checked already in the head

       // clean up
       smbc_closedir(dirfd);
   }
   else
   {
       if (errNum == EPERM || errNum == EACCES) {
           if (checkPassword(m_current_url)) {
               redirection( m_current_url );
               finished();
               return;
           }
       }

       reportError(m_current_url, errNum);
       return;
   }

   listEntry(udsentry, true);
   finished();
}

