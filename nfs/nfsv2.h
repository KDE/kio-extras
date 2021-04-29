/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>,
    SPDX-FileCopyrightText: 2014 Mathias Tillman <master.homer@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KIO_NFSV2_H
#define KIO_NFSV2_H

#include "kio_nfs.h"

#define PORTMAP  //this seems to be required to compile on Solaris
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

class NFSProtocolV2 : public NFSProtocol
{
public:
    explicit NFSProtocolV2(NFSSlave* slave);
    ~NFSProtocolV2() override;

    bool isCompatible(bool& connectionError) override;
    bool isConnected() const override;

    void openConnection() override;
    void closeConnection() override;

    void put(const QUrl& url, int _mode, KIO::JobFlags _flags) override;
    void get(const QUrl& url) override;
    void listDir(const QUrl& url) override;
    void symlink(const QString& target, const QUrl& dest, KIO::JobFlags) override;
    void stat(const QUrl& url) override;
    void mkdir(const QUrl& url, int permissions) override;
    void del(const QUrl& url, bool isfile) override;
    void chmod(const QUrl& url, int permissions) override;
    void rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) override;

protected:
    void copySame(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) override;
    void copyFrom(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) override;
    void copyTo(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) override;

    NFSFileHandle lookupFileHandle(const QString& path) override;

private:
    bool create(const QString& path, int mode, int& rpcStatus, diropres& result);

    bool getAttr(const QString& path, int& rpcStatus, attrstat& result);

    bool lookupHandle(const QString& path, int& rpcStatus, diropres& result);

    bool symLinkTarget(const QString& path, int& rpcStatus, readlinkres& result, char* dataBuffer);

    // Calls @remove, but with dummy rpcStatus and result arguments
    bool remove(const QString& path);
    bool remove(const QString& path, int& rpcStatus, nfsstat& result);

    // Calls @rename, but with dummy rpcStatus and result arguments
    bool rename(const QString& src, const QString& dest);
    bool rename(const QString& src, const QString& dest, int& rpcStatus, nfsstat& result);

    bool setAttr(const QString& path, const sattr& attributes, int& rpcStatus, nfsstat& result);

    bool symLink(const QString& target, const QString& dest, int& rpcStatus, nfsstat& result);

    // UDS helper functions
    void completeUDSEntry(KIO::UDSEntry& entry, const fattr& attributes);
    void completeBadLinkUDSEntry(KIO::UDSEntry& entry, const fattr& attributes);

    CLIENT* m_mountClient;
    int m_mountSock;
    CLIENT* m_nfsClient;
    int m_nfsSock;

    timeval clnt_timeout;
};

#endif
