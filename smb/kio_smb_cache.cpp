/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_cache.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              our internal cache and the password caching daemon
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

#include "kio_smb.h"

//===========================================================================
// SMBSlave authentication and cache function implementations
//===========================================================================
bool SMBSlave::cache_get_AuthInfo(SMBAuthInfo& auth)
{
    kdDebug(KIO_SMB) << "SMBSlave::cache_get_AuthInfo for: " << auth.m_workgroup << "|" << auth.m_server << "|" << auth.m_share << endl;

    //first search our current cache...
    for( SMBAuthInfo* it = m_auth_cache.first();
         it; it = m_auth_cache.next() )
    {
        if( (it->m_server    == auth.m_server) &&
            (it->m_share     == auth.m_share))
        {
	  kdDebug(KIO_SMB) << "found in top level cache :\nusername="<<it->m_username<<", \npassword="<<it->m_passwd<<",\ndomain="<<it->m_domain<<endl;
            auth.m_username = it->m_username;
            auth.m_passwd   = it->m_passwd;
            auth.m_domain   = it->m_domain;
            return true;
        }
    }

    //now check the password caching daemon as the last resort
    //if it is there, put it in our cache
    AuthInfo kauth = cache_create_AuthInfo( auth );
    if( checkCachedAuthentication( kauth ) )
    {
      kdDebug(KIO_SMB) << "found in password caching daemon\nusername="<<kauth.username<<", \npassword="<<kauth.password<<endl;
        // extract domain
        if (kauth.username.contains(';')) {
          auth.m_domain = kauth.username.left(kauth.username.find(';')).local8Bit();
          auth.m_username =  kauth.username.right(kauth.username.length()
                                          -kauth.username.find(';')-1).local8Bit();
        }
        else {
          auth.m_username = kauth.username.local8Bit();
        }
        auth.m_passwd = kauth.password.local8Bit();
        //store the info for later lookups
	//  cache_set_AuthInfo( auth );
        return true;
    }

    kdDebug(KIO_SMB) << "auth not cached at all..." << endl;
    return false;
}


void SMBSlave::cache_clear_AuthInfo(const SMBAuthInfo& auth) 
{

    SMBAuthInfo* it = m_auth_cache.first();
    while( it )
    {
      if ( it->m_server == auth.m_server)
            m_auth_cache.remove();
      else if((auth.m_server.isEmpty()) && (it->m_workgroup == auth.m_workgroup))
        {
            it = m_auth_cache.current();
        }
      else 
	it = m_auth_cache.next();
    }

  KURL kurl;
  kurl.setProtocol( "smb" );
  kurl.setPath( auth.m_server + "/" + auth.m_share );
  kdDebug(KIO_SMB) << "cache_clear_AuthInfo get CacheKey" << endl;
  QString s =  createAuthCacheKey(kurl);
  kdDebug(KIO_SMB) << "cache_clear_AuthInfo Key is " <<s<< endl;
  // does not work !!
  //  if (!s.isEmpty())
  //  delCachedAuthentication(s );

}

void SMBSlave::cache_set_AuthInfo(const SMBAuthInfo& _auth,
                                  bool store_in_kdesu)
{
    kdDebug(KIO_SMB) << "cache_set_AuthInfo"<< endl;
    SMBAuthInfo* auth = new SMBAuthInfo;
    auth->m_passwd    = _auth.m_passwd;
    auth->m_server    = _auth.m_server;
    auth->m_share     = _auth.m_share;
    auth->m_username  = _auth.m_username;
    auth->m_workgroup = _auth.m_workgroup;

    m_auth_cache.prepend( auth );

    if( store_in_kdesu )
    {
        AuthInfo kauth = cache_create_AuthInfo( *auth );
        cacheAuthentication( kauth );
    }
}


int SMBSlave::cache_stat(const SMBUrl& url, struct stat* st)
{
    int result;
    SMBAuthInfo auth;
DO_STAT:
    result = smbc_stat(url.toSmbcUrl(), st);
    if ((result !=0) && (errno == EACCES))  {
      // if access denied, first open passDlg
      kdDebug(KIO_SMB) << "cache_stat auth ERROR"<<endl;
      m_current_url.getAuthInfo(auth);
      if (!authDlg(auth)) {
	cache_clear_AuthInfo(auth);
        error( ERR_ACCESS_DENIED, m_current_url.toKioUrl() );
	return result;
      }
      else {
	goto DO_STAT;
      }
    }
    return result;
}

AuthInfo SMBSlave::cache_create_AuthInfo( const SMBAuthInfo& auth )
{
    AuthInfo rval;
    rval.url.setProtocol( "smb" );

    if( auth.m_server.isEmpty() )
    {
        rval.url.setPath(auth.m_workgroup);
    }
    else
    {
        rval.url.setPath( auth.m_server + "/" + auth.m_share );
    }

    rval.username = auth.m_username;
    if (!auth.m_domain.isEmpty())
      rval.username.prepend(auth.m_domain + ";");
    rval.password = auth.m_passwd;
    rval.keepPassword = true;
    return rval;
}


