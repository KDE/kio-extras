/*
Copyright 2009  Carlo Segato <brandon.ml@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "kio_smb_win.h"
#include <KComponentData>
#include <KDebug>
#include <QCoreApplication>
#include <QFile>

static void createUDSEntry(const QString &name, KIO::UDSEntry &entry)
{
    entry.insert( KIO::UDSEntry::UDS_NAME, name );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500 );
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
}

static void createUDSEntryBrowse(const QString &name, KIO::UDSEntry &entry)
{
    entry.insert( KIO::UDSEntry::UDS_NAME, name );
    entry.insert( KIO::UDSEntry::UDS_ICON_NAME, "network-server" );
    entry.insert( KIO::UDSEntry::UDS_TARGET_URL, "smb://"+name ); 
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500 );
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
}

static DWORD checkAuth(const KUrl &url)
{
    NETRESOURCE nr;
    HANDLE hEnum;
    DWORD dwResult;

    memset((void*)&nr, '\0', sizeof(NETRESOURCE));
    nr.dwType=RESOURCETYPE_DISK;
    nr.lpLocalName=NULL;
    nr.lpRemoteName = (LPWSTR)QString("\\\\").append(url.host()).append(url.path().replace("/", "\\")).utf16();
    nr.lpProvider=NULL;

    dwResult = WNetAddConnection2(&nr, NULL, NULL, CONNECT_INTERACTIVE);

    return dwResult;
}

extern "C" {
    int KDE_EXPORT kdemain(int argc, char **argv)
    {
        QCoreApplication app(argc, argv);
        KComponentData componentData("kio_smb");
        if( argc != 4 )
        {
            kDebug(KIO_SMB) << "Usage: kio_smb protocol domain-socket1 domain-socket2"
                      << endl;
            return -1;
        }

        SMBSlave slave( argv[2], argv[3] );

        slave.dispatchLoop();

        return 0;
    }
}

SMBSlave::SMBSlave(const QByteArray &pool, const QByteArray &app)
    : ForwardingSlaveBase("smb", pool, app)
{
}

SMBSlave::~SMBSlave()
{
}

bool SMBSlave::rewriteUrl(const KUrl &url, KUrl &newUrl)
{
    newUrl.setProtocol("file");
    newUrl.setPath("//"+url.host()+url.path());
    return true;
}

void SMBSlave::enumerateResources(LPNETRESOURCE lpnr, bool show_servers)
{
    KIO::UDSEntry entry;
    HANDLE hEnum;
    DWORD dwResult, dwResultEnum;
    DWORD cbBuffer = 16384;     // 16K is a good size
    DWORD cEntries = -1;        // enumerate all possible entries
    LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
    
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, lpnr, &hEnum);
   
    if (dwResult != NO_ERROR) {
        kWarning()<<"WnetOpenEnum failed with error"<<dwResult;
        return;
    }

    lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);

    do {
        ZeroMemory(lpnrLocal, cbBuffer);

        dwResultEnum = WNetEnumResource(hEnum, &cEntries, lpnrLocal, &cbBuffer);

        if (dwResultEnum == NO_ERROR) {
            for (int i = 0; i < cEntries; i++) {
                if (!show_servers || (show_servers && lpnrLocal[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)) {
                    QString rname = QString::fromUtf16( reinterpret_cast<ushort*>( lpnrLocal[i].lpRemoteName ) );
                    if (rname != "tsclient") {
                        if (show_servers) {
                            createUDSEntryBrowse(rname.section("\\", -1), entry);
                        } else {
                            createUDSEntry(rname.section("\\", -1), entry);
                        }
                        listEntry( entry, false );
                        entry.clear();
                    }
                }
                if(show_servers && lpnrLocal[i].dwDisplayType != RESOURCEDISPLAYTYPE_SERVER) {
                    enumerateResources(&lpnrLocal[i], show_servers);
                }
            }
        } else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
            kWarning()<<"WNetEnumResource failed with error"<<dwResultEnum;
            break;
        }
    } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    GlobalFree((HGLOBAL) lpnrLocal);

    dwResult = WNetCloseEnum(hEnum);
}

void SMBSlave::listDir(const KUrl &url)
{
    if (!url.path().isEmpty() && url.path() != "/") {
        return KIO::ForwardingSlaveBase::listDir(url);
    }

    NETRESOURCE nr;

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_DISK;
    nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpRemoteName = !url.host().isEmpty() ? (LPWSTR)QString("\\\\").append(url.host()).utf16() : NULL;
    nr.lpLocalName = NULL;
    nr.lpProvider = NULL;
    
    enumerateResources(&nr, url.host().isEmpty() ? true : false);

    listEntry( KIO::UDSEntry(), true );
    finished();
}

void SMBSlave::stat(const KUrl &url)
{
    DWORD res = checkAuth(url);

    if (!url.path().isEmpty() && url.path() != "/") {
        return KIO::ForwardingSlaveBase::stat(url);
    }

    KIO::UDSEntry entry;
    createUDSEntry(url.host(), entry);    
    statEntry( entry );

    finished();
}
