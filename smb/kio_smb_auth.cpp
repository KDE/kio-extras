/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_auth.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              SMB directory access
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

#include <qdir.h>
#include <stdlib.h>

// call for libsmbclient
//==========================================================================
void auth_smbc_get_data(const char *server,const char *share,
                        char *workgroup, int wgmaxlen,
                        char *username, int unmaxlen,
                        char *password, int pwmaxlen)
//==========================================================================
{
    G_TheSlave->auth_smbc_get_data(server, share,
                                   workgroup,wgmaxlen,
                                   username, unmaxlen,
                                   password, pwmaxlen);
}

//--------------------------------------------------------------------------
void SMBSlave::auth_smbc_get_data(const char *server,const char *share,
                                  char *workgroup, int wgmaxlen,
                                  char *username, int unmaxlen,
                                  char *password, int pwmaxlen)
//--------------------------------------------------------------------------
{
    //check this to see if we "really" need to authenticate...
    SMBUrlType t = m_current_url.getType();
    if( t == SMBURLTYPE_ENTIRE_NETWORK )
    {
        kdDebug(KIO_SMB) << "we don't really need to authenticate for this top level url, returning" << endl;
        return;
    }
    kdDebug(KIO_SMB) << "AAAAAAAAAAAAAA auth_smbc_get_dat: set user=" << username << ", workgroup=" << workgroup
                     << " server=" << server << ", share=" << share << endl;

    QString s_server = QString::fromUtf8(server);
    QString s_share = QString::fromUtf8(share);
    workgroup[wgmaxlen - 1] = 0;
    QString s_workgroup = QString::fromUtf8(workgroup);
    username[unmaxlen - 1] = 0;
    QString s_username = QString::fromUtf8(username);
    password[pwmaxlen - 1] = 0;
    QString s_password = QString::fromUtf8(password);

    KIO::AuthInfo info;
    info.url = KURL("smb:///");
    info.url.setHost(s_server);
    info.url.setPath("/" + s_share);

    info.username = s_username;
    info.password = s_password;
    info.verifyPath = true;

    if ( !checkCachedAuthentication( info ) )
    {
        // ok, we do not know the password. Let's try anonymous before we try for real
        info.username = "anonymous";
        info.password = QString::null;
        strncpy(username, info.username.utf8(),unmaxlen - 1);
        strncpy(password, info.password.utf8(),pwmaxlen - 1);
    } else {
        strncpy(username, info.username.utf8(),unmaxlen - 1);
        strncpy(password, info.password.utf8(),pwmaxlen - 1);
        kdDebug(KIO_SMB) << "got password through cache" << endl;
    }
}

//--------------------------------------------------------------------------
// Initalizes the smbclient library
//
// Returns: 0 on success -1 with errno set on error
bool SMBSlave::auth_initialize_smbc()
{
    kdDebug() << "auth_initialize_smbc " << endl;
    if(m_initialized_smbc == false)
    {
        //check for $HOME/.smb/smb.conf, the library dies without it...
        //create it with a sane default if it's not there
        bool mksmbdir = false, mksmbconf = false;
        QDir dir = QDir::home();

        if( dir.cd( ".smb" ) )
        {
            if( !dir.exists( "smb.conf" ) )
            {
                kdDebug(KIO_SMB) << "need to create the smb.conf file" << endl;
                mksmbconf = true;
            }
        }
        else
        {
            kdDebug(KIO_SMB) << "need to create the .smb dir and the smb.conf file" << endl;
            mksmbdir = true;
            mksmbconf = true;
        }

        if( mksmbdir )
        {
            dir.mkdir( ".smb" );
            dir.cd( ".smb" );
        }

        if( mksmbconf )
        {
            //copy our default workgroup to the smb.conf file
            QFile conf( dir.absPath() + "/smb.conf" );
            if( conf.open( IO_WriteOnly ) )
            {
                QTextStream output( &conf );
                output << "[global]" << endl;
                output << "\tworkgroup = " << m_default_workgroup << endl;
                conf.close();
            }
            else
            {
                SlaveBase::error(ERR_INTERNAL,
                    i18n("You are missing your $HOME/.smb/smb.conf file, and we could not create it.\n"
                         "Please manually create it to enable the smb ioslave to operate correctly.\n"
                         "The smb.conf file could look like:\n"
                         "[global]\n"
                         "workgroup= <YOUR_DEFAULT_WORKGROUP>"));
                return false;
            }
        }

        kdDebug() << "smbc_init call" << endl;
        int debug_level = 0;
#ifndef NDEBUG
        debug_level = 100;
#endif

        if(smbc_init(::auth_smbc_get_data,debug_level) == -1)
        {
            SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to initialize"));
            return false;
        }

        m_initialized_smbc = true;
    }

    return true;
}

