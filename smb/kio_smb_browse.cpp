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

#include "kio_smb.h"
#include "kio_smb_internal.h"

using namespace KIO;


//---------------------------------------------------------------------------
bool SMBSlave::browse_stat_path(const SMBUrl& url, UDSEntry& udsentry)
  // Returns: true on success, false on failure
{
  UDSAtom     udsatom;

  // realy needed ?
  // memset(&st,0,sizeof(st));
OPEN_STAT:
   if(cache_stat(url, &st) == 0)
   {
      if(S_ISDIR(st.st_mode))
      {
         kdDebug(KIO_SMB) << "SMBSlave::browse_stat_path is DIR"<< endl;
         // Directory
         udsatom.m_uds  = KIO::UDS_FILE_TYPE;
         udsatom.m_long = S_IFDIR;
         udsentry.append(udsatom);
      }
      else if(S_ISREG(st.st_mode))
      {
         kdDebug(KIO_SMB) << "SMBSlave::browse_stat_path is FILE"<< endl;
         // Regular file
         udsatom.m_uds  = KIO::UDS_FILE_TYPE;
         udsatom.m_long = S_IFREG;
         udsentry.append(udsatom);
      }
      else
      {
         kdDebug(KIO_SMB)<<"SMBSlave::browse_stat_path mode: "<<st.st_mode<<endl;
         error(ERR_INTERNAL, TEXT_UNKNOWN_ERROR);
         return false;
      }

      udsatom.m_uds  = KIO::UDS_FILE_TYPE;
      udsatom.m_long = st.st_mode;
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_SIZE;
      udsatom.m_long = st.st_size;
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_USER;
      uid_t uid = st.st_uid;
      struct passwd *user = getpwuid( uid );
      if ( user ) {
         udsatom.m_str = user->pw_name;
      }
      else
         udsatom.m_str = QString::number( uid );
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_GROUP;
      gid_t gid = st.st_gid;
      struct group *grp = getgrgid( gid );
      if ( grp ) {
         udsatom.m_str = grp->gr_name;
      }
      else
         udsatom.m_str = QString::number( gid );
      udsentry.append(udsatom);

      udsatom.m_uds  = KIO::UDS_ACCESS;
      udsatom.m_long = st.st_mode;
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
      switch(errno)
      {
      case EBUSY:
               break;  //hmmm, otherwise the whole dir isn't listed (caused e.g. by pagefile.sys), aleXXX
      case ENOENT:
      case ENOTDIR:
      case EFAULT:
         error(ERR_DOES_NOT_EXIST, url.toKioUrl());
         break;
      case EPERM:
      case EACCES:
         error( ERR_ACCESS_DENIED, url.toKioUrl() );
         break;
      case ENOMEM:
         error(ERR_OUT_OF_MEMORY, TEXT_OUT_OF_MEMORY);
      case EBADF:
         error(ERR_INTERNAL, "BAD Filediscriptor");
      default:
         kdDebug(KIO_SMB)<<"SMBSlave::browse_stat_path errno: "<<errno<< endl;
         error(ERR_INTERNAL, TEXT_UNKNOWN_ERROR);
      }

      kdDebug(KIO_SMB) << "SMBSlave::browse_stat_path ERROR!!"<< endl;
      return false;
    }

   return true;
}



//===========================================================================
// TODO: Add stat cache
void SMBSlave::stat( const KURL& kurl )
{
  kdDebug(KIO_SMB) << "SMBSlave::stat on"<< endl;
  // make a valid URL
  KURL url = checkURL(kurl);

  m_current_url.fromKioUrl( url );


  UDSAtom     udsatom;
  UDSEntry    udsentry;
  // Set name
  udsatom.m_uds = KIO::UDS_NAME;
  udsatom.m_str = kurl.fileName();
  udsentry.append( udsatom );
    

  switch(m_current_url.getType())
    {
    case SMBURLTYPE_UNKNOWN:
      error(ERR_MALFORMED_URL,m_current_url.toKioUrl());
      return;

    case SMBURLTYPE_ENTIRE_NETWORK:
    case SMBURLTYPE_WORKGROUP_OR_SERVER:
      udsatom.m_uds = KIO::UDS_FILE_TYPE;
      udsatom.m_long = S_IFDIR;
      udsentry.append(udsatom);
      break;

    case SMBURLTYPE_SHARE_OR_PATH:
      if (browse_stat_path(m_current_url, udsentry))
	break;
      else {
	kdDebug(KIO_SMB) << "SMBSlave::stat ERROR!!"<< endl;
	return;
      }
    default:
      kdDebug(KIO_SMB) << "SMBSlave::stat UNKNOWN " << url.url() << endl;
      return;
    }

  statEntry(udsentry);
  finished();

}

//===========================================================================
// TODO: complete checking
const KURL SMBSlave::checkURL(const KURL& kurl) {
  // smb:/ normaly have no userinfo
  // we must redirect ourself to remove the username and password
  if (kurl.url().contains('@') && !kurl.url().contains("smb://")) {
    KURL url(kurl);
    url.setPath("/"+kurl.url().right( kurl.url().length()-kurl.url().find('@') -1));
    QString userinfo = kurl.url().mid(5, kurl.url().find('@')-5);
    if(userinfo.contains(':'))  {
      url.setUser(userinfo.left(userinfo.find(':')));
      url.setPass(userinfo.right(userinfo.length()-userinfo.find(':')-1));
    }
    else {
      url.setUser(userinfo);
    }
    return url;
  }
  // no emtpy path
  QString path = kurl.path();
  if (path.isEmpty())
    {
      KURL url(kurl);
      url.setPath("/");
      return url;
    }

  return kurl;
}



//===========================================================================
// TODO: Add dir cache
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


   m_current_url.fromKioUrl( kurl );


   int                 dirfd;
   struct smbc_dirent  *dirp = NULL;
   UDSEntry    udsentry;
   UDSAtom     atom;
   bool cancel = false;
   OPEN_DIR:
   ;
   dirfd = smbc_opendir( m_current_url.toSmbcUrl());
   kdDebug(KIO_SMB) << "SMBSlave::listDir open " << kurl.url() << endl;
   if(dirfd >= 0)
   {
      while(1)
      {
         dirp = smbc_readdir(dirfd);
         if(dirp == NULL)
         {
            break;
         }
         // Set name
         atom.m_uds = KIO::UDS_NAME;
         atom.m_str = dirp->name;
         udsentry.append( atom );
         if (((!m_showHiddenShares) && (atom.m_str.right(1)=="$"))
             || (atom.m_str=="$IPC")
             || (atom.m_str==".")
             || (atom.m_str==".."))
         {
//            fprintf(stderr,"----------- hide: -%s-\n",dirp->name);
            // do nothing and hide the hidden shares
         }
         else if(dirp->smbc_type == SMBC_FILE)
         {
            // Set type
            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long = S_IFREG;
            udsentry.append( atom );

            // Set stat information
            m_current_url.append(dirp->name);
            browse_stat_path(m_current_url, udsentry);
            m_current_url.truncate();

            // Call base class to list entry
            listEntry(udsentry, false);
         }
         else if(dirp->smbc_type == SMBC_DIR)
         {
            // Set type
            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long = S_IFDIR;
            udsentry.append( atom );

            // Set stat information
            if(strcmp(dirp->name,".") &&
               strcmp(dirp->name,".."))
            {
               m_current_url.append(dirp->name);
               browse_stat_path(m_current_url, udsentry);
               m_current_url.truncate();
            }

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

            // remember the workgroup
            // we don't use it
            //	      cache_add_workgroup(dirp->name);
            // Call base class to list entry
            listEntry(udsentry, false);
         }
         else
         {
            kdDebug(KIO_SMB) << "SMBSlave::listDir SMBC_UNKNOWN :" << dirp->name << endl;
            // TODO: we don't handle SMBC_IPC_SHARE, SMBC_PRINTER_SHARE
            //       SMBC_LINK, SMBC_COMMS_SHARE
            //SlaveBase::error(ERR_INTERNAL, TEXT_UNSUPPORTED_FILE_TYPE);
            // continue;
         }
         udsentry.clear();
      }

      // clean up
      smbc_closedir(dirfd);
   }
   else
   {
      switch(errno)
      {
      case ENOENT:
      case ENOTDIR:
      case EFAULT:
         error(ERR_DOES_NOT_EXIST, m_current_url.toKioUrl());
         break;
      case EPERM:
      case EACCES:
         // if access denied, first open passDlg
         if ((errno == EPERM) || (errno ==  EACCES))
         {
            SMBAuthInfo auth;
            m_current_url.getAuthInfo(auth);
            if (!authDlg(auth))
            {
               cache_clear_AuthInfo(m_current_url.getAuthInfo());
               error(ERR_ACCESS_DENIED, m_current_url.toKioUrl());
               return;
            }
            else
               goto OPEN_DIR;
         }
         break;
      case ENOMEM:
         error(ERR_OUT_OF_MEMORY, TEXT_OUT_OF_MEMORY);
         break;
      case EUCLEAN:
         error(ERR_INTERNAL, TEXT_SMBC_INIT_FAILED);
         break;
      case ENODEV:
         error(ERR_INTERNAL, TEXT_NOSRV_WG);
         break;
      default:
         error(ERR_INTERNAL, TEXT_UNKNOWN_ERROR);
      }
      return;
   }

   listEntry(udsentry, true);
   finished();
}
