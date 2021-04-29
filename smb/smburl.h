/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        smburl.h
//
// Abstract:    Utility classes used by SMBSlave
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//              Frank Schwanz <schwanz@fh-brandenburg.de>
//---------------------------------------------------------------------------
//
// SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
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

#ifndef KIO_SMB_INTERNAL_H_INCLUDED
#define KIO_SMB_INTERNAL_H_INCLUDED

#include <QByteArray>
#include <QUrl>

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

/**
 * Class to handle URL's
 * it can convert QUrl to smbUrl
 * and Handle UserInfo
 * it also check the correctness of the URL
 */
class SMBUrl : public QUrl
{
public:
    SMBUrl();
    SMBUrl(const SMBUrl &);
    SMBUrl(const QUrl &kurl);
    ~SMBUrl();

    SMBUrl &operator=(const SMBUrl &);

    /**
     * Appends the specified file and dir to this SMBUrl
     * "smb://server/share" --> "smb://server/share/filedir"
     */
    void addPath(const QString &filedir);

    void cdUp();

    /**
     *   Returns the type of this SMBUrl:
     *   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
     *   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
     *   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
     *   URLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
     */
    SMBUrlType getType() const;

    void setPass(const QString &_txt) {
        QUrl::setPassword(_txt);
        updateCache();
    }
    void setUser(const QString &_txt) {
        QUrl::setUserName(_txt);
        updateCache();
    }
    void setHost(const QString &_txt) {
        QUrl::setHost(_txt);
        updateCache();
    }
    void setPath(const QString &_txt) {
        QUrl::setPath(_txt);
        updateCache();
    }

    /**
    * Return a URL that is suitable for libsmbclient
    */
    QByteArray toSmbcUrl() const {
        return m_surl;
    }

    /**
     * Returns the partial URL.
     */
    SMBUrl partUrl() const;

private:
    void updateCache();
    QByteArray m_surl;

    /**
     * Type of URL
     * @see _SMBUrlType
     */
    mutable SMBUrlType m_type = SMBURLTYPE_UNKNOWN;
};

#endif
