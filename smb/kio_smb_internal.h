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

/**
 *   Types of a SMBURL :
 *   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
 *   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
 *   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
 *   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
 */
typedef enum _SMBUrlType
{
    SMBURLTYPE_UNKNOWN = 0, SMBURLTYPE_ENTIRE_NETWORK = 1,
    SMBURLTYPE_WORKGROUP_OR_SERVER = 2, SMBURLTYPE_SHARE_OR_PATH = 3
}
SMBUrlType;

/**
 * AuthInfo is a bit different to KIO::AuthInfo
 * To use it for both urltypes smb:/ and smb:// workgroup will not be used for
 * searching in cache.
 * Workgroup is optional.
 * In KURL are :
 * server and share           = path (right ?? i think share should be realm)
 * userinfo (<domain;]user>)  = user
 * password                   = password
 *
 * Could be removed if we only use KIO caching.
 */
struct SMBAuthInfo
{
    QCString m_workgroup;
    QCString m_server;
    QCString m_share;
    QCString m_username;
    QCString m_passwd;
    QCString m_domain;
};



//===========================================================================
/**
 * Class to handle URL's
 * it can convert KURL to smbUrl 
 * and Handle UserInfo
 * it also check the correctness of the URL 
 */
class SMBUrl
{
    /**
     * Type of URL 
     * @see _SMBUrlType 
     */
    SMBUrlType m_type;
    QString m_kio_url;
    QString m_smbc_url;
    QString m_user;
    QString m_password;
    QString m_userdomain;

    int m_workgroup_index;
    int m_workgroup_len;

public:
    SMBUrl();
    SMBUrl(const KURL & kurl);

    /**
     * set the URL from KURL, all attributes will be updated
     */
    void fromKioUrl(const KURL & kurl);

    /**
     * Appends the specified file and dir to this SMBUrl
     * "smb://server/share" --> "smb://server/share/filedir"
     */
    SMBUrl & append(const char * filedir);


    /**
     *   Returns the type of this SMBUrl:
     *   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
     *   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
     *   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
     *   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
     */
    SMBUrlType getType();


    /**
     * Returns the workgroup if it given in url
     */
    QString getWorkgroup() const;

    /**
     * Returns path after workgroup
     */
    QString getServerShareDir() const;

    /**
     * Description : extract the domain and the username from userinfo
     * Parameter :   userinfo = [domain;]<user>[:password]
     */
    void setUserInfo(const QString & userinfo);


    /**
     * Return a URL that is suitable for libsmbclient
     */
    QCString toSmbcUrl() const;

    /**
     * Return a that is suitable for kio framework
     */
    const QString & toKioUrl() const;

    /*
     * Truncates one file/dir level
     * "smb://server/share/filedir" --> "smb://server/share"
     */
    void truncate();

     /**
      * Setter for m_password
      */
     void setPassword(const QString & _password);

    /**
     * Returns the username if given in KURL or updated through passdialog
     */
    QString getUser() const;

    /**
     * Returns the password if given in KURL or updated through passdialog
     */
    QString getPassword() const;

    /**
     * Returns the domain if given in KURL or updated through passdialog
     */
    QString getUserDomain() const;

    /**
     * Return a SMBAuthInfo of url
     */
    void getAuthInfo(SMBAuthInfo & auth);

    /**
     * Return a SMBAuthInfo of url
     */
    SMBAuthInfo getAuthInfo();

};


#endif

