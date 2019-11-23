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
//     a copy from https://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "kio_smb_internal.h"
#include "kio_smb.h"


#include <kconfig.h>
#include <QDir> // for QDir::cleanPath


//===========================================================================
// SMBUrl Function Implementation
//===========================================================================

//-----------------------------------------------------------------------
SMBUrl::SMBUrl()
{
    m_type = SMBURLTYPE_UNKNOWN;
}

//-----------------------------------------------------------------------
SMBUrl::SMBUrl(const QUrl& kurl)
    : QUrl(kurl)
  //-----------------------------------------------------------------------
{
    updateCache();
}

SMBUrl::SMBUrl(const SMBUrl& other)
    : QUrl(other),
      m_surl(other.m_surl),
      m_type(other.m_type)
{
}

SMBUrl& SMBUrl::operator=(const SMBUrl&) = default;

//-----------------------------------------------------------------------
void SMBUrl::addPath(const QString &filedir)
{
    if(path().length() > 0 && path().at(path().length() - 1) != QLatin1Char('/')) {
        QUrl::setPath(path() + QLatin1Char('/') + filedir);
    } else {
        QUrl::setPath(path() + filedir);
    }
    updateCache();
}

//-----------------------------------------------------------------------
bool SMBUrl::cd(const QString &filedir)
{
    if (filedir == "..") {
        setUrl(KIO::upUrl(*this).url());
    } else {
        setUrl(filedir);
    }
    updateCache();
    return true;
}

//-----------------------------------------------------------------------
void SMBUrl::updateCache()
  //-----------------------------------------------------------------------
{
    QUrl::setPath(QDir::cleanPath(path()));

    // SMB URLs are UTF-8 encoded
    qCDebug(KIO_SMB) << "updateCache " << QUrl::path();

    if ( QUrl::url() == "smb:/" )
      m_surl = "smb://";
    else
      m_surl = toString(QUrl::PrettyDecoded).toUtf8();
    
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

    if (scheme() != "smb")
    {
        m_type = SMBURLTYPE_UNKNOWN;
        return m_type;
    }

    if (path().isEmpty() || path(QUrl::FullyDecoded) == "/")
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

SMBUrl SMBUrl::partUrl() const
{
    if (m_type == SMBURLTYPE_SHARE_OR_PATH && !fileName().isEmpty()) {
        SMBUrl url (*this);
        url.setPath(path() + QLatin1String(".part"));
        return url;
    }

    return SMBUrl();
}
