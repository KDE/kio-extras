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
    int             n;
    int             dstflags;
    int             srcfd = -1;
    int             dstfd = -1;
    int             processed_size = 0;
    unsigned char   buf[MAX_XFER_BUF_SIZE];

    kdDebug(KIO_SMB) << "SMBSlave::copy with src = " << ksrc << "and dest = " << kdst << endl;

    // setup urls
    src = ksrc;
    dst = kdst;

    // Obtain information about source
    if(cache_stat(src, &st ) == -1)
    {
        if ( errno == EACCES )
        {
            error( KIO::ERR_ACCESS_DENIED, src.prettyURL());
        }
        else
        {
             error( KIO::ERR_DOES_NOT_EXIST, src.prettyURL());
        }
        return;
    }
    if ( S_ISDIR( st.st_mode ) )
    {
        error( KIO::ERR_IS_DIRECTORY, src.prettyURL() );
        return;
    }
    totalSize(st.st_size);

    // Check to se if the destination exists
    if(cache_stat(dst, &st) != -1)
    {
        if(S_ISDIR(st.st_mode))
        {
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.prettyURL());
	    return;
        }
        if(!overwrite)
        {
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.prettyURL());
	    return;
	}
    }

    // Open the source file
    srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if(srcfd < 0)
    {
        if(errno == EACCES)
        {
            error( KIO::ERR_ACCESS_DENIED, src.prettyURL() );
        }
        else
        {
            error( KIO::ERR_DOES_NOT_EXIST, src.prettyURL() );
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
            error(KIO::ERR_WRITE_ACCESS_DENIED, dst.prettyURL());
        }
        else
        {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, dst.prettyURL());
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
                error( KIO::ERR_COULD_NOT_WRITE, dst.prettyURL());
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
            error( KIO::ERR_COULD_NOT_READ, src.prettyURL());
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
            error( KIO::ERR_COULD_NOT_WRITE, dst.prettyURL());
	    return;
        }
    }

    finished();
}

//===========================================================================
void SMBSlave::del( const KURL &kurl, bool isfile)
{
    kdDebug(KIO_SMB) << "SMBSlave::del on " << kurl << endl;
    m_current_url = kurl;

    if(isfile)
    {
        // Delete file
        kdDebug(KIO_SMB) << "SMBSlave:: unlink " << kurl << endl;
        if(smbc_unlink(m_current_url.toSmbcUrl()) == -1)
        {
            switch(errno)
            {
            case EISDIR:
                error( KIO::ERR_IS_DIRECTORY, m_current_url.prettyURL());
                break;
            default:
                reportError(kurl);
            }
        }
    }
    else
    {
        kdDebug(KIO_SMB) << "SMBSlave:: rmdir " << kurl << endl;
        // Delete directory
        if(smbc_rmdir(m_current_url.toSmbcUrl()) == -1)
        {
            reportError(kurl);
        }
    }

    finished();
}

//===========================================================================
void SMBSlave::mkdir( const KURL &kurl, int permissions )
{
    kdDebug(KIO_SMB) << "SMBSlave::mkdir on " << kurl << endl;
    m_current_url = kurl;

    if(smbc_mkdir(m_current_url.toSmbcUrl(), 0777) != 0)
    {
        if (errno == EEXIST) {
            if(cache_stat(m_current_url, &st ) == 0)
            {
                if(S_ISDIR(st.st_mode ))
                {
                    error( KIO::ERR_DIR_ALREADY_EXIST, m_current_url.prettyURL());
                }
            }
            else
            {
                error( KIO::ERR_FILE_ALREADY_EXIST, m_current_url.prettyURL());
            }
        } else
            reportError(kurl);
	kdDebug(KIO_SMB) << "SMBSlave::mkdir exit with error " << kurl << endl;
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

    kdDebug(KIO_SMB) << "SMBSlave::rename, old name = " << ksrc << ", new name = " << kdest << endl;

    src = ksrc;
    dst = kdest;

    // Check to se if the destination exists

    kdDebug(KIO_SMB) << "SMBSlave::rename stat dst" << endl;
    if(cache_stat(dst, &st) != -1)
    {
        if(S_ISDIR(st.st_mode))
        {
	    kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_DIR_ALREADY_EXIST" << endl;
            error( KIO::ERR_DIR_ALREADY_EXIST, dst.prettyURL());
	    finished();
	    return;
        }
        if(!overwrite)
        {
	    kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_FILE_ALREADY_EXIST" << endl;
            error( KIO::ERR_FILE_ALREADY_EXIST, dst.prettyURL());
	    finished();
	    return;
	}
    }
    kdDebug(KIO_SMB ) << "smbc_rename " << src.toSmbcUrl() << " " << dst.toSmbcUrl() << endl;
    if(smbc_rename(src.toSmbcUrl(), dst.toSmbcUrl())!=0)
    {
        kdDebug(KIO_SMB ) << "failed " << perror << endl;
      switch(errno)
      {
        case ENOENT:
          if(cache_stat(src, &st) == -1)
          {
              if(errno == EACCES)
	      {
	        kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_ACCESS_DENIED" << endl;
                error(KIO::ERR_ACCESS_DENIED, src.prettyURL());
              }
              else
              {
		kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_DOES_NOT_EXIST" << endl;
                error(KIO::ERR_DOES_NOT_EXIST, src.prettyURL());
              }
          }
          break;

        case EACCES:
        case EPERM:
  	  kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_ACCESS_DENIED" << endl;
          error( KIO::ERR_ACCESS_DENIED, dst.prettyURL() );
          break;

        default:
  	  kdDebug(KIO_SMB) << "SMBSlave::rename KIO::ERR_CANNOT_RENAME" << endl;
          error( KIO::ERR_CANNOT_RENAME, src.prettyURL() );

      }

      kdDebug(KIO_SMB) << "SMBSlave::rename exit with error" << endl;
      return;
    }

    kdDebug(KIO_SMB ) << "everything fine\n";
    finished();
}


