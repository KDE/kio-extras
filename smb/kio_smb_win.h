#ifndef KIO_SMB_WIN_H
#define KIO_SMB_WIN_H

#include <kio/slavebase.h>
#include <kio/forwardingslavebase.h>

#define KIO_SMB                     7106
 
class SMBSlave : public KIO::ForwardingSlaveBase
{
    public:
        SMBSlave(const QByteArray &pool, const QByteArray &app);
        ~SMBSlave();
        bool rewriteUrl(const KUrl &url, KUrl &newUrl);
        void listDir(const KUrl &url);
        void stat(const KUrl &url);

};

#endif
