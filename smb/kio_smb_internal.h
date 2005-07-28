/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_internal.h
//
// Abstract:    Utility classes used by SMBSlave
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//              Frank Schwanz <schwanz@fh-brandenburg.de>
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

#ifndef KIO_SMB_INTERNAL_H_INCLUDED
#define KIO_SMB_INTERNAL_H_INCLUDED

#include <kio/authinfo.h>
//Added by qt3to4:
#include <Q3CString>

/**
 *   Types of a SMBURL :
 *   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
 *   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
 *   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
 *   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
 */
enum SMBUrlType {
    SMBURLTYPE_UNKNOWN = 0, SMBURLTYPE_ENTIRE_NETWORK = 1,
    SMBURLTYPE_WORKGROUP_OR_SERVER = 2, SMBURLTYPE_SHARE_OR_PATH = 3
};


//===========================================================================
/**
 * Class to handle URL's
 * it can convert KURL to smbUrl
 * and Handle UserInfo
 * it also check the correctness of the URL
 */
class SMBUrl : public KURL
{


public:
    SMBUrl();
    SMBUrl(const KURL & kurl);

    /**
     * Appends the specified file and dir to this SMBUrl
     * "smb://server/share" --> "smb://server/share/filedir"
     */
    void addPath(const QString &filedir);

    bool cd(const QString &dir);

    /**
     *   Returns the type of this SMBUrl:
     *   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
     *   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
     *   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
     *   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
     */
    SMBUrlType getType() const;

    void setPass( const QString& _txt ) { KURL::setPass(_txt); updateCache(); }
    void setUser( const QString& _txt ) { KURL::setUser(_txt); updateCache(); }
    void setHost( const QString& _txt ) { KURL::setHost(_txt); updateCache(); }

    /**
     * Returns the workgroup if it given in url
     */
//    QString getWorkgroup() const;

    /**
     * Returns path after workgroup
     */
//    QString getServerShareDir() const;

     /**
     * Return a URL that is suitable for libsmbclient
     */
    Q3CString toSmbcUrl() const { return m_surl; }

private:
    /**
     * Change from QString to QCString (MS Windows's character encoding)
     */
    Q3CString fromUnicode( const QString &_str ) const;

    void updateCache();
    Q3CString m_surl;

    /**
     * Type of URL
     * @see _SMBUrlType
     */
    mutable SMBUrlType m_type;
};


#endif

