/////////////////////////////////////////////////////////////////////////////
//                                                                         
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_internal.cpp  
//                                                                         
// Abstract:    Utility class implementation used by SMBSlave
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
#include "kio_smb_internal.h"


//===========================================================================
// SMBUrl Function Implementation
//===========================================================================


//-----------------------------------------------------------------------
SMBUrl::SMBUrl()
//-----------------------------------------------------------------------
{
    m_type = SMBURLTYPE_UNKNOWN;
}

//-----------------------------------------------------------------------
SMBUrl::SMBUrl(const KURL& kurl)
//-----------------------------------------------------------------------
{
    fromKioUrl(kurl);
}


//-----------------------------------------------------------------------
SMBUrl& SMBUrl::append(const char* filedir)
// Appends the specified file and dir to this SMBUrl
// "smb://server/share" --> "smb://server/share/filedir"
//-----------------------------------------------------------------------
{
    if(m_smbc_url[m_smbc_url.length()-1] != '/')
        m_smbc_url.append("/");
    m_smbc_url.append(filedir);

    if(m_kio_url[m_kio_url.length()-1] != '/')
        m_kio_url.append("/");
    m_kio_url.append(filedir);

    return *this;
}


//-----------------------------------------------------------------------
void SMBUrl::setUser(const QString &userinfo)
//-----------------------------------------------------------------------
{

      // extract domain
      if (userinfo.contains(';')) {
          m_userdomain = userinfo.left(userinfo.find(';'));
          m_user =  userinfo.right(userinfo.length()-userinfo.find(';')-1);
      }
      else
          m_user =  userinfo;
}

//-----------------------------------------------------------------------
void SMBUrl::fromKioUrl(const KURL& kurl)
//-----------------------------------------------------------------------
{
    m_type     = SMBURLTYPE_UNKNOWN;
    m_kio_url  = kurl.prettyURL();
    m_user     = "";
    m_userdomain = "";
    m_password   = "";
    // remove userinfo from m_kio_url
    if (m_kio_url.contains('@')) {
      int pos_at = m_kio_url.find('@');
      int pos_slash = m_kio_url.findRev('/', pos_at);
      int pos_dp = m_kio_url.findRev(':', pos_at);
      // set password
      if (pos_dp>5)
          m_password = m_kio_url.mid(pos_dp+1,pos_at-pos_dp-1);
      // set userinfo
      if (pos_dp>5)
          setUser(m_kio_url.mid(pos_slash+1, pos_dp-pos_slash-1));
      else
          setUser(m_kio_url.mid(pos_slash+1, pos_at-pos_slash-1));
      m_kio_url.remove(pos_slash+1, pos_at-pos_slash);
      // on "smb://" the password is stored int pass()
      if (!kurl.pass().isEmpty())
        m_password = kurl.pass();
    }
    m_smbc_url = m_kio_url;
    m_workgroup_index = m_kio_url.find('/') + 1;
    m_workgroup_len = m_kio_url.find('/',m_workgroup_index);

    if(m_workgroup_len < 0)
    {
        m_workgroup_len = 0; //m_kio_url.length();
    }
    m_workgroup_len = m_workgroup_len - m_workgroup_index;

    SMBUrlType t = getType();
    if( t == SMBURLTYPE_WORKGROUP_OR_SERVER )
    {
        int end_index = m_smbc_url.length() - 1;
        if( m_smbc_url[ end_index ] == '/' )
            m_smbc_url.remove( end_index, 1 );
    }
		// remove Workgroup if smb:/workgroup/host/share
		if ( ((t == SMBURLTYPE_WORKGROUP_OR_SERVER) ||
         (t == SMBURLTYPE_SHARE_OR_PATH)) &&
        !m_smbc_url.contains("smb://") &&
        (m_smbc_url.find('/', 5)>0)    &&
        (m_smbc_url.find('/', 5)<(int)m_smbc_url.length()-1)) {
          int pos_slash_host = m_kio_url.find('/', 5);
          int pos_slash_wg = m_smbc_url.findRev('/',  pos_slash_host-1);
          m_smbc_url.remove( pos_slash_wg+1, pos_slash_host-pos_slash_wg);
        }
		if (!m_smbc_url.contains("smb://"))
	    m_smbc_url = m_smbc_url.insert(4,'/');

}

//-----------------------------------------------------------------------   
QCString SMBUrl::toSmbcUrl() const
// Return a URL that is suitable for libsmbclient
//----------------------------------------------------------------------- 
{
		kdDebug(KIO_SMB) << "toSmbcURL, returning: " << QString(m_smbc_url.local8Bit()) << endl;
    return m_smbc_url.local8Bit();

}

//----------------------------------------------------------------------- 
const QString& SMBUrl::toKioUrl() const
// Return a URL that is suitable for kio framework
//----------------------------------------------------------------------- 
{
    return m_kio_url;
}

//-----------------------------------------------------------------------
SMBUrlType SMBUrl::getType()
// Returns the type of this SMBUrl:
//   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
//   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
//   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
//   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
//-----------------------------------------------------------------------
{
    int pos1;
    int pos2;
    
    if(m_type != SMBURLTYPE_UNKNOWN)
    {
        return m_type;
    }

    // check to see if we can find a "smb:/"
    pos1 = m_kio_url.find("smb:/");
    if(pos1 == -1)
    {
        m_type = SMBURLTYPE_UNKNOWN;
        return m_type;
    }

    // Check for entire network exactly "smb:/"
    if(m_kio_url.length() == 5)
    {
        m_type = SMBURLTYPE_ENTIRE_NETWORK;
        return m_type;
    }

    // skip the "smb://"
	  if (m_kio_url.contains("smb://"))
	    pos1 = 6;
		else  // skip "smb:/"
			pos1 = 5;

    // Check for the workgroup
    pos2 = m_kio_url.find('/',pos1);
    if (!m_kio_url.contains("smb://") && (pos2!=-1))  // means smb:/workgroup/host
      pos2 = m_kio_url.find('/',pos2+1);

    if ((pos2 == -1) || (pos2 == (int)(m_kio_url.length()-1)))
    {   // smb://host/
        m_type = SMBURLTYPE_WORKGROUP_OR_SERVER;
        return m_type;
    }


    // Check for the path if we get this far
    m_type = SMBURLTYPE_SHARE_OR_PATH;

    return m_type;
}


//-----------------------------------------------------------------------
void SMBUrl::truncate()
// Truncates one file/dir level
// "smb://server/share/filedir" --> "smb://server/share"
//-----------------------------------------------------------------------
{
    m_smbc_url.truncate(m_smbc_url.findRev('/'));
    m_kio_url.truncate(m_kio_url.findRev('/'));
}

//-----------------------------------------------------------------------
QString SMBUrl::getWorkgroup() const
//-----------------------------------------------------------------------
{
    return m_kio_url.mid(m_workgroup_index, m_workgroup_len).local8Bit();
}


//-----------------------------------------------------------------------
QString SMBUrl::getServerShareDir() const
//-----------------------------------------------------------------------
{
    return m_kio_url.right(m_kio_url.length() - (m_workgroup_index + m_workgroup_len)).local8Bit();
}

//-----------------------------------------------------------------------
QString SMBUrl::getUser() const
//-----------------------------------------------------------------------
{
    return m_user;
}

//-----------------------------------------------------------------------
QString SMBUrl::getPassword() const
//-----------------------------------------------------------------------
{
    return m_password;
}
//-----------------------------------------------------------------------
QString SMBUrl::getUserDomain() const
//-----------------------------------------------------------------------
{
    return m_userdomain;
}

