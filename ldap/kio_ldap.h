#ifndef __LDAP_H__
#define __LDAP_H__

#include <qstring.h>


#include <kio/slavebase.h>
#include <kio/authinfo.h>

#define LDAP_DEPRECATED 1 /* Needed for ldap_simple_bind_s with openldap >= 2.3.x */
#include <lber.h>
#include <ldap.h>
#include <kabc/ldapurl.h>

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

    int saslInteract( void *in );

  private:

    QString mHost;
    int mPort;
    QString mUser;
    QString mPassword;
    LDAP *mLDAP;
    int mVer, mSizeLimit, mTimeLimit;
    bool mTLS;
    bool mAuthSASL;
    QString mMech,mRealm,mBindName;
    bool mCancel, mFirstAuth;
    
    void controlsFromMetaData( LDAPControl ***serverctrls, 
      LDAPControl ***clientctrls );
    void addControlOp( LDAPControl ***pctrls, const QString &oid,
      const QByteArray &value, bool critical );
    void addModOp( LDAPMod ***pmods, int mod_type, 
      const QString &attr, const QByteArray &value );
    void LDAPEntry2UDSEntry( const QString &dn, KIO::UDSEntry &entry, 
      const KABC::LDAPUrl &usrc, bool dir=false );
    int asyncSearch( KABC::LDAPUrl &usrc );
    
    QByteArray LDAPEntryAsLDIF( LDAPMessage *msg );
    void LDAPErr( const KUrl &url, int err = LDAP_SUCCESS );
    void changeCheck( KABC::LDAPUrl &url );

    void fillAuthInfo( KIO::AuthInfo &info );
};

#endif
