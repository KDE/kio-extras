
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

#include <qtextcodec.h>

#include <kglobal.h>

#include "kio_smb.h"
#include "kio_smb_internal.h"

using namespace KIO;

int SMBSlave::cache_stat(const SMBUrl &url, struct stat* st )
{
    int result = smbc_stat(url.toSmbcUrl(), st);
    kdDebug(KIO_SMB) << "smbc_stat " << url.toSmbcUrl() << " " << errno << " " << result << endl;
    return result;
}

//---------------------------------------------------------------------------
bool SMBSlave::browse_stat_path(const SMBUrl& _url, UDSEntry& udsentry, bool ignore_errors)
  // Returns: true on success, false on failure
{
    UDSAtom     udsatom;

    SMBUrl url = _url;

 again:
   if(cache_stat(url, &st) == 0)
   {
      if(!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
      {
         kdDebug(KIO_SMB)<<"SMBSlave::browse_stat_path mode: "<<st.st_mode<<endl;
         warning(i18n("%1:\n"
                      "Unknown file type, neither dir or file.").arg(url.prettyURL()));
         return false;
      }

      udsatom.m_uds  = KIO::UDS_FILE_TYPE;
      udsatom.m_long = st.st_mode & S_IFMT;
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_SIZE;
      udsatom.m_long = st.st_size;
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_USER;
      uid_t uid = st.st_uid;
      struct passwd *user = getpwuid( uid );
      if ( user )
          udsatom.m_str = user->pw_name;
      else
          udsatom.m_str = QString::number( uid );
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_GROUP;
      gid_t gid = st.st_gid;
      struct group *grp = getgrgid( gid );
      if ( grp )
          udsatom.m_str = grp->gr_name;
      else
          udsatom.m_str = QString::number( gid );
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_ACCESS;
      udsatom.m_long = st.st_mode & 07777;
      udsentry.append(udsatom);

      udsatom.m_uds  = UDS_MODIFICATION_TIME;
      udsatom.m_long = st.st_mtime;
      udsentry.append(udsatom);

      udsatom.m_uds  = UDS_ACCESS_TIME;
      udsatom.m_long = st.st_atime;
      udsentry.append(udsatom);

      udsatom.m_uds  = UDS_CREATION_TIME;
      udsatom.m_long = st.st_ctime;
      udsentry.append(udsatom);

   }
   else
   {
       if (!ignore_errors) {
           if (errno == EPERM || errno == EACCES)
               if (checkPassword(url))
                   goto again;

           reportError(url);
       } else if (errno == ENOENT || errno == ENOTDIR) {
           warning(i18n("File does not exist: %1").arg(url.url()));
       }
       kdDebug(KIO_SMB) << "SMBSlave::browse_stat_path ERROR!!"<< endl;
       return false;
   }

   return true;
}

//===========================================================================
void SMBSlave::stat( const KURL& kurl )
{
    kdDebug(KIO_SMB) << "SMBSlave::stat on"<< endl;
    // make a valid URL
    KURL url = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (url != kurl)
    {
        redirection(url);
        finished();
        return;
    }

    m_current_url = url;

    UDSAtom     udsatom;
    UDSEntry    udsentry;
    // Set name
    udsatom.m_uds = KIO::UDS_NAME;
    udsatom.m_str = kurl.fileName();
    udsentry.append( udsatom );

    switch(m_current_url.getType())
    {
    case SMBURLTYPE_UNKNOWN:
        error(ERR_MALFORMED_URL,m_current_url.url());
        return;

    case SMBURLTYPE_ENTIRE_NETWORK:
    case SMBURLTYPE_WORKGROUP_OR_SERVER:
        udsatom.m_uds = KIO::UDS_FILE_TYPE;
        udsatom.m_long = S_IFDIR;
        udsentry.append(udsatom);
        break;

    case SMBURLTYPE_SHARE_OR_PATH:
        if (browse_stat_path(m_current_url, udsentry, false))
            break;
        else {
            kdDebug(KIO_SMB) << "SMBSlave::stat ERROR!!"<< endl;
            finished();
            return;
        }
    default:
        kdDebug(KIO_SMB) << "SMBSlave::stat UNKNOWN " << url.url() << endl;
        finished();
        return;
    }

    statEntry(udsentry);
    finished();
}

//===========================================================================
// TODO: complete checking
KURL SMBSlave::checkURL(const KURL& kurl) const {

    kdDebug(KIO_SMB) << "checkURL " << kurl << endl;
    QString surl = kurl.url();
    if (surl.startsWith("smb:/")) {
        if (surl.length() == 5) // just the above
            return kurl; // unchanged

        if (surl.at(5) != '/') {
            surl = "smb://" + surl.mid(5);
            kdDebug(KIO_SMB) << "checkURL return1 " << surl << " " << KURL(surl) << endl;
            return KURL(surl);
        }
    }

    // smb:/ normaly have no userinfo
    // we must redirect ourself to remove the username and password
    if (surl.contains('@') && !surl.contains("smb://")) {
        KURL url(kurl);
        url.setPath("/"+kurl.url().right( kurl.url().length()-kurl.url().find('@') -1));
        QString userinfo = kurl.url().mid(5, kurl.url().find('@')-5);
        if(userinfo.contains(':'))  {
            url.setUser(userinfo.left(userinfo.find(':')));
            url.setPass(userinfo.right(userinfo.length()-userinfo.find(':')-1));
        } else {
            url.setUser(userinfo);
        }
        kdDebug() << "checkURL return2 " << url << endl;
        return url;
    }
    // no emtpy path
    QString path = kurl.path();
    if (path.isEmpty())
    {
        KURL url(kurl);
        url.setPath("/");
        kdDebug() << "checkURL return3 " << url << endl;
        return url;
    }

    kdDebug() << "checkURL return3 " << kurl << endl;
    return kurl;
}

void SMBSlave::reportError(const SMBUrl &url)
{
    switch(errno)
    {
    case EBUSY:
        break;  //hmmm, otherwise the whole dir isn't listed (caused e.g. by pagefile.sys), aleXXX
    case ENOENT:
        if (url.getType() == SMBURLTYPE_ENTIRE_NETWORK) {
            error( ERR_SLAVE_DEFINED, i18n("Couldn't find any workgroups in your local network."));
            break;
        }
    case ENOTDIR:
    case EFAULT:
        error(ERR_DOES_NOT_EXIST, url.prettyURL());
        break;
    case EPERM:
    case EACCES:
        error( ERR_ACCESS_DENIED, url.prettyURL() );
        break;
    case EIO:
        error( ERR_CONNECTION_BROKEN, url.url());
        break;
    case ENOMEM:
        error( ERR_OUT_OF_MEMORY, url.url() );
        break;
    case EUCLEAN:
        error( ERR_INTERNAL, i18n("libsmbclient failed to initialize"));
        break;
    case ENODEV:
        error( ERR_SLAVE_DEFINED, i18n("Share could not be found on given server"));
        break;
    case EBADF:
        error( ERR_INTERNAL, "BAD File descriptor");
        break;
    default:
        error( ERR_INTERNAL, i18n("Unknown error condition in stat: %1").arg(strerror(errno)));
    }
}

//===========================================================================
void SMBSlave::listDir( const KURL& kurl )
{
   kdDebug(KIO_SMB) << "SMBSlave::listDir on " << kurl.url() << endl;

   // check (correct) URL
   KURL url = checkURL(kurl);
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
   UDSAtom     atom;

 again:
   dirfd = smbc_opendir( m_current_url.toSmbcUrl() );
   kdDebug(KIO_SMB) << "SMBSlave::listDir open " << m_current_url.toSmbcUrl() << " " << m_current_url.getType() << " " << dirfd << endl;
   if(dirfd >= 0)
   {
       do {
           kdDebug(KIO_SMB) << "smbc_readdir " << endl;
           dirp = smbc_readdir(dirfd);
           if(dirp == 0)
               break;

           // Set name
           atom.m_uds = KIO::UDS_NAME;
           QString dirpName = QString::fromUtf8( dirp->name );
           kdDebug(KIO_SMB) << "dirp->name " <<  dirp->name  << " " << dirpName << endl;
           atom.m_str = dirpName;
           udsentry.append( atom );
           if (atom.m_str.upper()=="$IPC" || atom.m_str=="." || atom.m_str == ".." ||
               atom.m_str.upper() == "ADMIN$" || atom.m_str.lower() == "printer$")
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
               atom.m_uds = KIO::UDS_FILE_TYPE;
               atom.m_long = S_IFDIR;
               udsentry.append( atom );

               // Set permissions
               atom.m_uds  = KIO::UDS_ACCESS;
               atom.m_long = (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
               udsentry.append(atom);

               if (dirp->smbc_type == SMBC_SERVER) {
                   atom.m_uds = KIO::UDS_URL;
                   // QString workgroup = m_current_url.host().upper();
                   KURL u("smb:/");
                   u.setHost(dirpName);
                   atom.m_str = u.url();

                   // when libsmbclient knows
                   // atom.m_str = QString("smb://%1?WORKGROUP=%2").arg(dirpName).arg(workgroup.upper());
                   kdDebug(KIO_SMB) << "list item " << atom.m_str << endl;
                   udsentry.append(atom);
               }

               // Call base class to list entry
               listEntry(udsentry, false);
            }
           else if(dirp->smbc_type == SMBC_WORKGROUP)
           {
               // Set type
               atom.m_uds = KIO::UDS_FILE_TYPE;
               atom.m_long = S_IFDIR;
               udsentry.append( atom );

               // Set permissions
               atom.m_uds  = KIO::UDS_ACCESS;
               atom.m_long = (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
               udsentry.append(atom);

               // Call base class to list entry
               listEntry(udsentry, false);
           }
           else
           {
               kdDebug(KIO_SMB) << "SMBSlave::listDir SMBC_UNKNOWN :" << dirpName << endl;
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
       if (errno == EPERM || errno == EACCES)
           if (checkPassword(m_current_url))
               goto again;

       reportError(m_current_url);
       finished();
       return;
   }

   listEntry(udsentry, true);
   finished();
}

