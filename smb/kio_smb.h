/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#ifndef KIO_SMB_H_INCLUDED
#define KIO_SMB_H_INCLUDED

#include <config-smb.h>

#include "smb-logsettings.h"

//--------------
// KF includes
//--------------
#include <kio/global.h>
#include <kio/slavebase.h>

//-----------------------------
// Standard C library includes
//-----------------------------
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>

//-----------------------------
// Qt includes
//-----------------------------

#include <QDateTime>
#include <QLoggingCategory>
#include <QObject>
#include <QUrl>

//-------------------------------
// Samba client library includes
//-------------------------------
extern "C" {
#include <libsmbclient.h>
}

//---------------------------
// kio_smb internal includes
//---------------------------
#include "smburl.h"
#include "smbauthenticator.h"
#include "smbcontext.h"

using namespace KIO;
class SMBSlave;

class SlaveFrontend : public SMBAbstractFrontend
{
public:
    SlaveFrontend(SMBSlave &slave);
    bool checkCachedAuthentication(AuthInfo &info) override;
private:
    SMBSlave &m_slave;
};

class SMBSlave : public QObject, public KIO::SlaveBase
{
    Q_OBJECT
    friend class SMBCDiscoverer;
    friend class SMBResumeIO;
    SlaveFrontend m_frontend { *this };
    SMBContext m_context { new SMBAuthenticator(m_frontend) };

private:

    class SMBError
    {
    public:
        int kioErrorId;
        QString errorString;
    };

    /**
     * we store the current url, it's needed for
     * callback authorization method
     */
    SMBUrl m_current_url;

    /**
     * From Controlcenter, show SHARE$ or not
     */
    //    bool m_showHiddenShares;     //currently unused, Alex <neundorf@kde.org>

    /**
     * libsmbclient need global variables to store in,
     * else it crashes on exit next method after use cache_stat,
     * looks like gcc (C/C++) failure
     */
    struct stat st {
    };

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

    int checkPassword(SMBUrl &url);

    //---------------------------------------------
    // Cache functions (kio_smb_auth.cpp)
    //---------------------------------------------

    // Stat methods

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
    int browse_stat_path(const SMBUrl &url, UDSEntry &udsentry);

    /**
     * Description :  call smbc_stat and return stats of the url
     * Parameter :    SMBUrl the url to stat
     * Return :       stat* of the url
     * Note :         it has some problems with stat in method, looks like
     *                something leave(or removed) on the stack. If your
     *                method segfault on returning try to change the stat*
     *                variable
     */
    static int cache_stat(const SMBUrl &url, struct stat *st);

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
    QUrl checkURL(const QUrl &kurl) const;

    void reportError(const SMBUrl &url, const int errNum);
    void reportWarning(const SMBUrl &url, const int errNum);

public:
    //-----------------------------------------------------------------------
    // Overwritten functions from the base class that define the operation of
    // this slave. (See the base class headerfile slavebase.h for more
    // details)
    //-----------------------------------------------------------------------

    // Functions overwritten in kio_smb.cpp
    SMBSlave(const QByteArray &pool, const QByteArray &app);
    ~SMBSlave() override;

    // Functions overwritten in kio_smb_browse.cpp
    void listDir(const QUrl &url) override;
    void stat(const QUrl &url) override;

    // Functions overwritten in kio_smb_config.cpp
    void reparseConfiguration() override;

    // Functions overwritten in kio_smb_dir.cpp
    void copy(const QUrl &src, const QUrl &dst, int permissions, KIO::JobFlags flags) override;
    void del(const QUrl &kurl, bool isfile) override;
    void mkdir(const QUrl &kurl, int permissions) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;

    // Functions overwritten in kio_smb_file.cpp
    void get(const QUrl &kurl) override;
    void put(const QUrl &kurl, int permissions, KIO::JobFlags flags) override;
    void open(const QUrl &kurl, QIODevice::OpenMode mode) override;
    void read(KIO::filesize_t bytesRequested) override;
    void write(const QByteArray &fileData) override;
    void seek(KIO::filesize_t offset) override;
    void truncate(KIO::filesize_t length);
    void close() override;

    // Functions not implemented  (yet)
    // virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);
    // virtual void openConnection();
    // virtual void closeConnection();
    // virtual void slave_status();
    void special(const QByteArray &) override;

protected:
    void virtual_hook(int id, void *data) override;

private:
    SMBError errnumToKioError(const SMBUrl &url, const int errNum);
    void smbCopy(const QUrl &src, const QUrl &dst, int permissions, KIO::JobFlags flags);
    void smbCopyGet(const QUrl &ksrc, const QUrl &kdst, int permissions, KIO::JobFlags flags);
    void smbCopyPut(const QUrl &ksrc, const QUrl &kdst, int permissions, KIO::JobFlags flags);
    bool workaroundEEXIST(const int errNum) const;
    int statToUDSEntry(const QUrl &url, const struct stat &st, KIO::UDSEntry &udsentry);
    void fileSystemFreeSpace(const QUrl &url);

    /**
     * Used in open(), read(), write(), and close()
     * FIXME Placing these in the private section above causes m_openUrl = kurl
     * to fail in SMBSlave::open. Need to find out why this is.
     */
    int m_openFd;
    SMBUrl m_openUrl;

    const bool m_enableEEXISTWorkaround; /* Enables a workaround for some broken libsmbclient versions */
    // Close without calling finish(). Use this to close after error.
    void closeWithoutFinish();

    // Apply mtime if modified metadata is set. This callsback with a utbuf
    // with modtime accordingly set. The callback should implement the actual apply.
    template<typename UTimeFunction>
    void applyMTime(UTimeFunction &&callback)
    {
        const QString mtimeStr = metaData("modified");
        if (mtimeStr.isEmpty()) {
            return;
        }
        qCDebug(KIO_SMB_LOG) << "modified:" << mtimeStr;

        const QDateTime dateTime = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dateTime.isValid()) {
            struct utimbuf utbuf {
            };
            utbuf.modtime = dateTime.toSecsSinceEpoch(); // modification time
            callback(utbuf);
        }
    }

    void applyMTimeSMBC(const SMBUrl &url)
    {
#ifdef HAVE_UTIME_H // smbc_utime is conditional inside the libsmb headers
        applyMTime([url](struct utimbuf utbuf) {
            struct stat st {
            };
            if (cache_stat(url, &st) == 0) {
                utbuf.actime = st.st_atime; // access time, unchanged
                smbc_utime(url.toSmbcUrl(), &utbuf);
            }
        });
#endif
    }
};

//===========================================================================
// Main slave entrypoint (see kio_smb.cpp)
extern "C" {
    int kdemain(int argc, char **argv);
}

#endif //#endif KIO_SMB_H_INCLUDED
