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

#include <kconfig.h>
#include <QDir>
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
        kDebug(KIO_SMB) << "we don't really need to authenticate for this top level url, returning" << endl;
        return;
    }
    kDebug(KIO_SMB) << "AAAAAAAAAAAAAA auth_smbc_get_dat: set user=" << username << ", workgroup=" << workgroup
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
    info.url = KUrl("smb:///");
    info.url.setHost(s_server);
    info.url.setPath('/' + s_share);

    info.username = s_username;
    info.password = s_password;
    info.verifyPath = true;

    kDebug(KIO_SMB) << "libsmb-auth-callback URL:" << info.url << endl;

    if ( !checkCachedAuthentication( info ) )
    {
        if ( m_default_user.isEmpty() )
        {
            // ok, we do not know the password. Let's try anonymous before we try for real
            info.username = "anonymous";
            info.password.clear();
        }
        else
        {
            // user defined a default username/password in kcontrol; try this
            info.username = m_default_user;
            info.password = m_default_password;
        }

    } else
        kDebug(KIO_SMB) << "got password through cache" << endl;

    strncpy(username, info.username.toUtf8(), unmaxlen - 1);
    strncpy(password, info.password.toUtf8(), pwmaxlen - 1);
}

bool SMBSlave::checkPassword(SMBUrl &url)
{
    kDebug(KIO_SMB) << "checkPassword for " << url << endl;

    KIO::AuthInfo info;
    info.url = KUrl("smb:///");
    info.url.setHost(url.host());

    QString share = url.path();
    int index = share.indexOf('/', 1);
    if (index > 1)
        share = share.left(index);
    if (share.at(0) == '/')
        share = share.mid(1);
    info.url.setPath('/' + share);
    info.verifyPath = true;

    if ( share.isEmpty() )
        info.prompt = i18n(
            "<qt>Please enter authentication information for <b>%1</b></qt>" ,
                        url.host() );
    else
        info.prompt = i18n(
            "Please enter authentication information for:\n"
            "Server = %1\n"
            "Share = %2" ,
                        url.host() ,
                        share );

    info.username = url.user();
    kDebug(KIO_SMB) << "call openPasswordDialog for " << info.url << endl;

    if ( openPasswordDialog(info) ) {
        kDebug(KIO_SMB) << "openPasswordDialog returned " << info.username << endl;
        url.setUser(info.username);
        return true;
    }
    kDebug(KIO_SMB) << "no value from openPasswordDialog\n";
    return false;
}

//--------------------------------------------------------------------------
// Initalizes the smbclient library
//
// Returns: 0 on success -1 with errno set on error
bool SMBSlave::auth_initialize_smbc()
{
    SMBCCTX *smb_context = NULL;

    kDebug(KIO_SMB) << "auth_initialize_smbc " << endl;
    if(m_initialized_smbc == false)
    {
        kDebug(KIO_SMB) << "smbc_init call" << endl;
        KConfig cfg( "kioslaverc", KConfig::OnlyLocal);
        int debug_level = cfg.group( "SMB" ).readEntry( "DebugLevel", 0 );

#if 0
	/* old API initialisation routine does not allow to set flags */

        if(smbc_init(::auth_smbc_get_data,debug_level) == -1)
        {
            SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to initialize"));
            return false;
        }
#endif
	smb_context = smbc_new_context();
	if (smb_context == NULL) {
            SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to create context"));
	    return false;
	}

	smb_context->debug = debug_level;
	smb_context->callbacks.auth_fn = ::auth_smbc_get_data;

	if (!smbc_init_context(smb_context)) {
		smbc_free_context(smb_context, false);
		smb_context = NULL;
            	SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to initialize context"));
	    	return false;
	}

#if defined(SMB_CTX_FLAG_USE_KERBEROS) && defined(SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS)
	smb_context->flags |= SMB_CTX_FLAG_USE_KERBEROS | SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS;
#endif

	smbc_set_context(smb_context);

        m_initialized_smbc = true;
    }

    return true;
}

