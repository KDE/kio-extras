#ifndef __LDAP_H__
#define __LDAP_H__

#include <qstring.h>
#include <qvaluelist.h>

#include <kio/slavebase.h>
#include <kio/authinfo.h>

#include <lber.h>
#include <ldap.h>
#include <kabc/ldapurl.h>

class LDAPProtocol : public KIO::SlaveBase
{
  public:
    LDAPProtocol( const QCString &protocol, const QCString &pool, const QCString &app );
    virtual ~LDAPProtocol();
    
    virtual void setHost( const QString& host, int port,
                          const QString& user, const QString& pass );

    virtual void openConnection();
    virtual void closeConnection();
    
    virtual void get( const KURL& url );
    virtual void stat( const KURL& url );
    virtual void listDir( const KURL& url );
    virtual void del( const KURL& url, bool isfile );
    virtual void put( const KURL& url, int permissions, bool overwrite, bool resume );

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
    
    QCString LDAPEntryAsLDIF( LDAPMessage *msg );
    void checkErr( const KURL &_url );
    void LDAPErr( int error, const QString &msg );
    void changeCheck( KABC::LDAPUrl &url );

    void fillAuthInfo( KIO::AuthInfo &info );
};

#endif
