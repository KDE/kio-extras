#include "kio_smb_win.h"
#include <KComponentData>
#include <KDebug>
#include <QCoreApplication>

#include <windows.h>
#include <winnetwk.h>
#include <QFile>

static void createUDSEntry(const QString &name, KIO::UDSEntry &entry)
{
    entry.insert( KIO::UDSEntry::UDS_NAME, name );
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


void SMBSlave::listDir(const KUrl &url)
{
    if (!url.path().isEmpty() && url.path() != "/") {
        return KIO::ForwardingSlaveBase::listDir(url);
    }

    KIO::UDSEntry entry;
    HANDLE hEnum;
    DWORD dwResult, dwResultEnum;
    DWORD cbBuffer = 16384;     // 16K is a good size
    DWORD cEntries = -1;        // enumerate all possible entries
    LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
    NETRESOURCE nr;
    
    //dwResult = checkAuth(url);

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_DISK;
    nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpRemoteName = (LPWSTR)QString("\\\\").append(url.host()).utf16();
    nr.lpLocalName = NULL;
    nr.lpProvider = NULL;
    
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
                        RESOURCETYPE_DISK,   // all resources
                        0,  // enumerate all resources
                        &nr,       // NULL first time the function is called
                        &hEnum);    // handle to the resource

    

    if (dwResult != NO_ERROR) {
        kWarning()<<"WnetOpenEnum failed with error"<<dwResult;
        return;
    }

    lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
    
    
    do {
        ZeroMemory(lpnrLocal, cbBuffer);

        dwResultEnum = WNetEnumResource(hEnum,  // resource handle
                                        &cEntries,      // defined locally as -1
                                        lpnrLocal,      // LPNETRESOURCE
                                        &cbBuffer);     // buffer size

        if (dwResultEnum == NO_ERROR) {
            for (int i = 0; i < cEntries; i++) {
                QString rname = QString::fromUtf16( reinterpret_cast<ushort*>( lpnrLocal[i].lpRemoteName ) );
                createUDSEntry(rname.section("\\", -1), entry);

                listEntry( entry, false );
                entry.clear();
            }
        } else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
            kWarning()<<"WNetEnumResource failed with error"<<dwResultEnum;
            break;
        }
    } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    GlobalFree((HGLOBAL) lpnrLocal);

    dwResult = WNetCloseEnum(hEnum);

    listEntry( entry, true );
    finished();
}

void SMBSlave::stat(const KUrl &url)
{
    DWORD res = checkAuth(url);

    if (!url.path().isEmpty() && url.path() != "/") {
        return KIO::ForwardingSlaveBase::listDir(url);
    }

    KIO::UDSEntry entry;
    createUDSEntry(url.host(), entry);    
    statEntry( entry );

    finished();
}
