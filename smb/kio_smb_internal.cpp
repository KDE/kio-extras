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

#include <qtextcodec.h>

#include <kconfig.h>
#include <kglobal.h>


//===========================================================================
// SMBUrl Function Implementation
//===========================================================================

void updateCache() {}

//-----------------------------------------------------------------------
SMBUrl::SMBUrl()
{
    m_type = SMBURLTYPE_UNKNOWN;
}

//-----------------------------------------------------------------------
SMBUrl::SMBUrl(const KURL& kurl)
    : KURL(kurl)
  //-----------------------------------------------------------------------
{
    updateCache();
}


//-----------------------------------------------------------------------
void SMBUrl::addPath(const QString &filedir)
{
    KURL::addPath(filedir);
    updateCache();
}

//-----------------------------------------------------------------------
bool SMBUrl::cd(const QString &filedir)
{
    if (!KURL::cd(filedir))
        return false;
    updateCache();
    return true;
}

//-----------------------------------------------------------------------
void SMBUrl::updateCache()
  //-----------------------------------------------------------------------
{
    // we have to use pretty here as smbc is unable to handle e.g. %20
    m_surl = fromUnicode(KURL::url());
    if (m_surl.left(5) == "smb:/" && m_surl.at(6) != '/')
        m_surl = "smb://" + m_surl.mid(6);

    m_type = SMBURLTYPE_UNKNOWN;
    // update m_type
    (void)getType();
}

//-----------------------------------------------------------------------
SMBUrlType SMBUrl::getType() const
  // Returns the type of this SMBUrl:
  //   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
  //   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
  //   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
  //   SMBURLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
  //-----------------------------------------------------------------------
{
    if(m_type != SMBURLTYPE_UNKNOWN)
        return m_type;

    if (protocol() != "smb")
    {
        m_type = SMBURLTYPE_UNKNOWN;
        return m_type;
    }

    if (path(1) == "/")
    {
        if (host().isEmpty())
            m_type = SMBURLTYPE_ENTIRE_NETWORK;
        else
            m_type = SMBURLTYPE_WORKGROUP_OR_SERVER;
        return m_type;
    }

    // Check for the path if we get this far
    m_type = SMBURLTYPE_SHARE_OR_PATH;

    return m_type;
}

void SMBUrl::getAuthInfo(SMBAuthInfo & auth) {
    /* TODO: find out what this is supposed to do
    auth.m_workgroup = getWorkgroup().local8Bit();
  QString servershare = getServerShareDir();
  int endshareidx=0;
  int endserveridx = servershare.find('/',3);
  kdDebug(KIO_SMB) << "endserveridx = "<< endserveridx<<", endshareidx="<<endshareidx << endl;
  if (endserveridx<=0) {
    auth.m_share     = "";
    endserveridx = servershare.length();
  }
  else {
    endshareidx = servershare.find('/',endserveridx+1 );
    if (endshareidx<=0)
      endshareidx = servershare.length();
    auth.m_share   = servershare.mid(endserveridx+1, endshareidx-endserveridx-1).local8Bit();
  }
  kdDebug(KIO_SMB) << "endserveridx = "<< endserveridx<<", endshareidx="<<endshareidx <<auth.m_share<< endl;
  auth.m_server    = servershare.mid(servershare.findRev('/',3)+1, endserveridx-servershare.findRev('/',3)-1).local8Bit();
  auth.m_domain    = getUserDomain().local8Bit();
  auth.m_username  = getUser().local8Bit();
  auth.m_passwd    = getPassword().local8Bit();
    */
}


SMBAuthInfo SMBUrl::getAuthInfo() {
  SMBAuthInfo sa;
  getAuthInfo(sa);
  return sa;
}

QCString SMBUrl::fromUnicode( const QString &_str ) const
{
    QCString _string;

    KConfig *cfg = new KConfig( "kioslaverc", true );
    cfg->setGroup( "Browser Settings/SMBro" );

    QString m_encoding = QTextCodec::codecForLocale()->name();
    QString default_encoding = cfg->readEntry( "Encoding", m_encoding.lower() );

    QTextCodec *codec = QTextCodec::codecForName( default_encoding.latin1() );
    if ( codec )
        _string = codec->fromUnicode( _str );
    else
        _string = _str.local8Bit();

    delete cfg;
    return _string;
}
