// SPDX-License-Identifier: MIT

#ifndef __info_h__
#define __info_h__

#include "kio_info_debug.h"

#include <kio/slavebase.h>

class InfoProtocol : public KIO::SlaveBase
{
public:

    InfoProtocol( const QByteArray &pool, const QByteArray &app );
    ~InfoProtocol() override = default;

    void get( const QUrl& url ) override;
    void stat( const QUrl& url ) override;
    void mimetype( const QUrl& url ) override;

protected:

    void decodeURL( const QUrl &url );
    void decodePath( QString path );

private:

    QString   m_page;
    QString   m_node;

    QString   m_perl;
    QString   m_infoScript;
    QString   m_infoConf;
    QString   m_cssLocation;
};

#endif // __info_h__
