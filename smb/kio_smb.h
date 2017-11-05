/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE
//
// File:        kio_smb.h
//
// Abstract:    The main kio slave class declaration.  For convenience,
//              in concurrent devlopment, the implementation for this class
//              is separated into several .cpp files -- the file containing
//              the implementation should be noted in the comments for each
//              member function.
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


#ifndef KIO_SMB_H_INCLUDED
#define KIO_SMB_H_INCLUDED

#include "config-smb.h"

//--------------
// KDE includes
//--------------
#include <kio/global.h>
#include <kio/slavebase.h>

//-----------------------------
// Standard C library includes
//-----------------------------
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

//-----------------------------
// Qt includes
//-----------------------------

#include <QObject>
#include <QUrl>
#include <QLoggingCategory>

//-------------------------------
// Samba client library includes
//-------------------------------
extern "C"
{
#include <libsmbclient.h>
}

//---------------------------
// kio_smb internal includes
//---------------------------
#include "kio_smb_internal.h"

#define MAX_XFER_BUF_SIZE           65534

// Categorized logger
Q_DECLARE_LOGGING_CATEGORY(KIO_SMB)

using namespace KIO;

//===========================================================================


class SMBSlave : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

private:
    class SMBError {
    public:
        int kioErrorId;
        QString errorString;
    };

    //---------------------------------------------------------------------
    // please make sure your private data does not duplicate existing data
    //---------------------------------------------------------------------
    bool     m_initialized_smbc;

    /**
     * From Controlcenter
     */
    QString  m_default_user;
//    QString  m_default_workgroup; //currently unused, Alex <neundorf@kde.org>
    QString  m_default_password;
    QString  m_default_encoding;

    /**
     * we store the current url, it's needed for
     * callback authorization method
     */
    SMBUrl   m_current_url;

    /**
     * From Controlcenter, show SHARE$ or not
     */
//    bool m_showHiddenShares;     //currently unused, Alex <neundorf@kde.org>

    /**
     * libsmbclient need global variables to store in,
     * else it crashes on exit next method after use cache_stat,
     * looks like gcc (C/C++) failure
     */
    struct stat st;

protected:
    //---------------------------------------------
    // Authentication functions (kio_smb_auth.cpp)
    //---------------------------------------------
    // (please prefix functions with auth)


    /**
     * Description :   Initializes the libsmbclient
     * Return :        true on success false with errno set on error
     */
    bool auth_initialize_smbc();

    bool checkPassword(SMBUrl &url);


    //---------------------------------------------
    // Cache functions (kio_smb_auth.cpp)
    //---------------------------------------------

    //Stat methods

    //-----------------------------------------
    // Browsing functions (kio_smb_browse.cpp)
    //-----------------------------------------
    // (please prefix functions with browse)

    /**
     * Description :  Return a stat of given SMBUrl. Calls cache_stat and
     *                pack it in UDSEntry. UDSEntry will not be cleared
     * Parameter :    SMBUrl the url to stat
     * Return :       cache_stat() return code
     */
    int browse_stat_path(const SMBUrl& url, UDSEntry& udsentry);

    /**
     * Description :  call smbc_stat and return stats of the url
     * Parameter :    SMBUrl the url to stat
     * Return :       stat* of the url
     * Note :         it has some problems with stat in method, looks like
     *                something leave(or removed) on the stack. If your
     *                method segfault on returning try to change the stat*
     *                variable
     */
    int cache_stat( const SMBUrl& url, struct stat* st );

    //---------------------------------------------
    // Configuration functions (kio_smb_config.cpp)
    //---------------------------------------------
    // (please prefix functions with config)


    //---------------------------------------
    // Directory functions (kio_smb_dir.cpp)
    //---------------------------------------
    // (please prefix functions with dir)


    //--------------------------------------
    // File IO functions (kio_smb_file.cpp)
    //--------------------------------------
    // (please prefix functions with file)

    //----------------------------
    // Misc functions (this file)
    //----------------------------


    /**
     * Description :  correct a given URL
     *                valid URL's are
     *
     *                smb://[[domain;]user[:password]@]server[:port][/share[/path[/file]]]
     *                smb:/[[domain;]user[:password]@][group/[server[/share[/path[/file]]]]]
     *                domain   = workgroup(domain) of the user
     *                user     = username
     *                password = password of useraccount
     *                group    = workgroup(domain) of server
     *                server   = host to connect
     *                share    = a share of the server (host)
     *                path     = a path of the share
     * Parameter :    QUrl the url to check
     * Return :       new QUrl if it is corrected. else the same QUrl
     */
    QUrl checkURL(const QUrl& kurl) const;

    void reportError(const SMBUrl& url, const int errNum);
    void reportWarning(const SMBUrl& url, const int errNum);

public:

    //-----------------------------------------------------------------------
    // smbclient authentication callback (note that this is called by  the
    // global ::auth_smbc_get_data() call.
    void auth_smbc_get_data(const char *server,const char *share,
                            char *workgroup, int wgmaxlen,
                            char *username, int unmaxlen,
                            char *password, int pwmaxlen);


    //-----------------------------------------------------------------------
    // Overwritten functions from the base class that define the operation of
    // this slave. (See the base class headerfile slavebase.h for more
    // details)
    //-----------------------------------------------------------------------

    // Functions overwritten in kio_smb.cpp
    SMBSlave(const QByteArray& pool, const QByteArray& app);
    virtual ~SMBSlave();

    // Functions overwritten in kio_smb_browse.cpp
    virtual void listDir( const QUrl& url );
    virtual void stat( const QUrl& url );

    // Functions overwritten in kio_smb_config.cpp
    virtual void reparseConfiguration();

    // Functions overwritten in kio_smb_dir.cpp
    virtual void copy( const QUrl& src, const QUrl &dest, int permissions, KIO::JobFlags flags );
    virtual void del( const QUrl& kurl, bool isfile);
    virtual void mkdir( const QUrl& kurl, int permissions );
    virtual void rename( const QUrl& src, const QUrl& dest, KIO::JobFlags flags );

    // Functions overwritten in kio_smb_file.cpp
    virtual void get( const QUrl& kurl );
    virtual void put( const QUrl& kurl, int permissions, KIO::JobFlags flags );
    virtual void open( const QUrl& kurl, QIODevice::OpenMode mode );
    virtual void read( KIO::filesize_t bytesRequested );
    virtual void write( const QByteArray &fileData );
    virtual void seek( KIO::filesize_t offset );
    virtual void close();

    // Functions not implemented  (yet)
    //virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);
    //virtual void openConnection();
    //virtual void closeConnection();
    //virtual void slave_status();
    virtual void special( const QByteArray & );

protected:
    void virtual_hook(int id, void *data) Q_DECL_OVERRIDE;

private:
    SMBError errnumToKioError(const SMBUrl& url, const int errNum);
    void smbCopy(const QUrl& src, const QUrl &dest, int permissions, KIO::JobFlags flags);
    void smbCopyGet(const QUrl& src, const QUrl& dest, int permissions, KIO::JobFlags flags);
    void smbCopyPut(const QUrl& src, const QUrl& dest, int permissions, KIO::JobFlags flags);
    bool workaroundEEXIST(const int errNum) const;

    void fileSystemFreeSpace(const QUrl &url);

     /**
     * Used in open(), read(), write(), and close()
     * FIXME Placing these in the private section above causes m_openUrl = kurl
     * to fail in SMBSlave::open. Need to find out why this is.
     */
    int m_openFd;
    SMBUrl m_openUrl;

    const bool m_enableEEXISTWorkaround; /* Enables a workaround for some broken libsmbclient versions */
};

//==========================================================================
// the global libsmbclient authentication callback function
extern "C"
{

void auth_smbc_get_data(SMBCCTX * context,
                        const char *server,const char *share,
                        char *workgroup, int wgmaxlen,
                        char *username, int unmaxlen,
                        char *password, int pwmaxlen);

}


//===========================================================================
// Main slave entrypoint (see kio_smb.cpp)
extern "C"
{

int kdemain( int argc, char **argv );

}


#endif  //#endif KIO_SMB_H_INCLUDED
