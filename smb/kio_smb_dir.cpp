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


//===========================================================================
// TODO: add when libsmbclient supports it
void SMBSlave::copy( const KURL& ksrc,
                     const KURL& kdst, 
                     int permissions, 
                     bool overwrite)
{

    SMBUrl          src;
    SMBUrl          dst;
    mode_t          initialmode;
    time_t          starttime;
    time_t          lasttime;
    time_t          curtime;
    int             n;
    int             dstflags;
    int             srcfd = -1;
    int             dstfd = -1;
    int             processed_size = 0;
    unsigned char   buf[MAX_XFER_BUF_SIZE];
    
    kdDebug(KIO_SMB) << "SMBSlave::copy with src = " << ksrc.url() << "and dest = " << kdst.url() << endl;

    // setup times
    starttime = time(NULL);
    lasttime = starttime;

    // setup urls
    src.fromKioUrl(ksrc);
    dst.fromKioUrl(kdst);
    
    // Obtain information about source
    if(cache_stat(src, &st ) == -1)
    {
        if ( errno == EACCES )
        {
            error( KIO::ERR_ACCESS_DENIED, src.toKioUrl());
        }
        else
        {
             error( KIO::ERR_DOES_NOT_EXIST, src.toKioUrl());
        }
        return;
    }
    if ( S_ISDIR( st.st_mode ) )
    {
        error( KIO::ERR_IS_DIRECTORY, src.toKioUrl() );
        return;
    }
    totalSize(st.st_size);

    // Check to se if the destination exists
    if(cache_stat(dst, &st) != -1)
    {
        if(S_ISDIR(st.st_mode))
        {
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.toKioUrl());
	    return;
        }
        if(!overwrite)
        {
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.toKioUrl());
	    return;
	}
    }

    // Open the source file
    srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if(srcfd < 0)
    {
        if(errno == EACCES)
        {
            error( KIO::ERR_ACCESS_DENIED, src.toKioUrl() );
        }
        else
        {
            error( KIO::ERR_DOES_NOT_EXIST, src.toKioUrl() );
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
    if(!overwrite)
    {
        dstflags |= O_EXCL;
    }
    dstfd = smbc_open(dst.toSmbcUrl(), dstflags, initialmode);
    if(dstfd < 0)
    {
        if(errno == EACCES)
        {
            error(KIO::ERR_WRITE_ACCESS_DENIED, dst.toKioUrl());
        }
        else
        {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, dst.toKioUrl());
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
	        kdDebug(KIO_SMB) << "SMBSlave::copy copy now KIO::ERR_COULD_NOT_WRITE" << endl;
                error( KIO::ERR_COULD_NOT_WRITE, dst.toKioUrl());
                break;
            }
            
            processed_size += n;
            curtime = time(NULL);
            if(curtime - lasttime > 0)
            {
                processedSize(processed_size);
                speed(processed_size / (curtime - starttime));
                lasttime = curtime;
            }
	}
        else if(n == 0)
	{
	      break; // finished
	}
	else
	{
            error( KIO::ERR_COULD_NOT_READ, src.toKioUrl());
	    break;
        }
    }
        
    
    FINISHED:
    
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
            error( KIO::ERR_COULD_NOT_WRITE, dst.toKioUrl());
	    return;
        }
    }

    finished();
}

//===========================================================================
void SMBSlave::del( const KURL &kurl, bool isfile)
{
    kdDebug(KIO_SMB) << "SMBSlave::del on " << kurl.url() << endl;
    m_current_url.fromKioUrl( kurl );

    if(isfile)
    {
SMBC_DEL:
        // Delete file
        if(smbc_unlink(m_current_url.toSmbcUrl()) == -1)
        {
            switch(errno)
            {
            case EACCES:
            case EPERM:
	      cache_clear_AuthInfo(m_current_workgroup);
	      // if access denied, first open passDlg
	      if ((errno == EPERM) || (errno ==  EACCES)) {
		SMBAuthInfo auth;
		m_current_url.getAuthInfo(auth);
		if (!authDlg(auth)) {
		  error(ERR_ACCESS_DENIED, m_current_url.toKioUrl());
		  return;
		}
		else
		  goto SMBC_DEL;
	      }
	      break;
            case EISDIR:
                error( KIO::ERR_IS_DIRECTORY, m_current_url.toKioUrl());
                break;

            default:
                error( KIO::ERR_CANNOT_DELETE, m_current_url.toKioUrl());
            }
	    return;
        }
    }
    else
    {
        // Delete directory
        if(smbc_rmdir(m_current_url.toSmbcUrl()) == -1)
        {
            switch(errno)
            {
            case EACCES:
            case EPERM:
                error( KIO::ERR_ACCESS_DENIED, m_current_url.toKioUrl());
                break;

            default:
                error( KIO::ERR_COULD_NOT_RMDIR, m_current_url.toKioUrl());
                break;
            }
        }
	return;
    }

    finished();
}

//===========================================================================
void SMBSlave::mkdir( const KURL &kurl, int permissions )
{
    kdDebug(KIO_SMB) << "SMBSlave::mkdir on " << kurl.url() << endl;
    m_current_url.fromKioUrl( kurl );

SMBC_MKDIR:
    if(smbc_mkdir(m_current_url.toSmbcUrl(), 0777) != 0)
    {
        switch(errno)
        {
        case EACCES:
        case EPERM:
	  cache_clear_AuthInfo(m_current_workgroup);
	  // if access denied, first open passDlg
 	  if ((errno == EPERM) || (errno ==  EACCES)) {
	    SMBAuthInfo auth;
	    m_current_url.getAuthInfo(auth);
	    if (!authDlg(auth)) {
	      error(ERR_ACCESS_DENIED, m_current_url.toKioUrl());
	      return;
	    }
	    else
	      goto SMBC_MKDIR;
	  }
	  break;
        case EEXIST:
            if(cache_stat(m_current_url, &st ) == 0)
            {
                if(S_ISDIR(st.st_mode ))
                {
                    error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.toKioUrl());
                }
            }
            else
            {
                error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.toKioUrl());
            }
            break;

        default:
            error( KIO::ERR_COULD_NOT_MKDIR, m_current_url.toKioUrl());
            break;
        }
	return;
    }
    else
    {
        if(permissions != -1)
        {
            // TODO enable the following when complete
            //smbc_chmod( url.toSmbcUrl(), permissions );
        }
    }

    finished();
}


//===========================================================================
void SMBSlave::rename( const KURL& ksrc, const KURL& kdest, bool overwrite )
{

    SMBUrl      src;
    SMBUrl      dst;

    kdDebug(KIO_SMB) << "SMBSlave::rename, old name = " << ksrc.url() << ", new name = " << kdest.url() << endl;

    src.fromKioUrl(ksrc);
    dst.fromKioUrl(kdest);


    // Check to se if the destination exists
    if(cache_stat(dst, &st) != -1)
    {
        if(S_ISDIR(st.st_mode))
        {
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.toKioUrl());
	    return;
        }
        if(!overwrite)
        {
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.toKioUrl());
	    return;
	}
    }
    if(smbc_rename(src.toSmbcUrl(), dst.toSmbcUrl())!=0)
    {
      switch(errno)
      {
        case ENOENT:
          if(cache_stat(src, &st) == -1)
          {
              if(errno == EACCES)
	      {
                error(KIO::ERR_ACCESS_DENIED, src.toKioUrl());
              }
              else
              {
                error(KIO::ERR_DOES_NOT_EXIST, src.toKioUrl());
              }
          }
          break;

        case EACCES:
        case EPERM:
          error( KIO::ERR_ACCESS_DENIED, dst.toKioUrl() );
          break;

        default:
          error( KIO::ERR_CANNOT_RENAME, src.toKioUrl() );
      }
      return;
    }
    finished();
}


