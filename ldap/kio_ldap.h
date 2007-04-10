#ifndef __LDAP_H__
#define __LDAP_H__

#include <QString>


#include <kio/slavebase.h>
#include <kio/authinfo.h>

#include <kldap/ldapurl.h>
#include <kldap/ldapcontrol.h>
#include <kldap/ldapconnection.h>
#include <kldap/ldapdn.h>
#include <kldap/ldapoperation.h>

class LDAPProtocol : public KIO::SlaveBase
{
  public:
    LDAPProtocol( const QByteArray &protocol, const QByteArray &pool, const QByteArray &app );
    virtual ~LDAPProtocol();
    
    virtual void setHost( const QString& host, int port,
                          const QString& user, const QString& pass );

    virtual void openConnection();
    virtual void closeConnection();
    
    virtual void get( const KUrl& url );
    virtual void stat( const KUrl& url );
    virtual void listDir( const KUrl& url );
    virtual void del( const KUrl& url, bool isfile );
    virtual void put( const KUrl& url, int permissions, bool overwrite, bool resume );

  private:

    KLDAP::LdapConnection mConn;
    KLDAP::LdapOperation mOp;
    KLDAP::LdapServer mServer;
    bool mConnected;
    
    bool mCancel, mFirstAuth;
    
    void controlsFromMetaData( KLDAP::LdapControls &serverctrls,
      KLDAP::LdapControls &clientctrls );
    void LDAPEntry2UDSEntry( const KLDAP::LdapDN &dn, KIO::UDSEntry &entry,
      const KLDAP::LdapUrl &usrc, bool dir=false );
    int asyncSearch( KLDAP::LdapUrl &usrc, const QByteArray &cookie = "" );
    
//    int parsePageControl( LDAPMessage *result, QByteArray &cookie );
    void LDAPErr( int err = KLDAP_SUCCESS );
    void changeCheck( KLDAP::LdapUrl &url );
};

#endif
