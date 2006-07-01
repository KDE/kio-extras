#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/stat.h>

#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>

#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>

#ifdef HAVE_SASL_SASL_H //prefer libsasl2
#include <sasl/sasl.h>
#else
#ifdef HAVE_SASL_H
#include <sasl.h>
#endif
#endif
#include <kabc/ldif.h>

#include "kio_ldap.h"

using namespace KIO;
using namespace KABC;

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

/**
 * The main program.
 */
int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_ldap" );

  kDebug(7125) << "Starting " << getpid() << endl;

  if ( argc != 4 ) {
    kError() << "Usage kio_ldap protocol pool app" << endl;
    return -1;
  }

  // let the protocol class do its work
  LDAPProtocol slave( argv[1], argv[ 2 ], argv[ 3 ] );
  slave.dispatchLoop();

  kDebug( 7125 ) << "Done" << endl;
  return 0;
}

/**
 * Initialize the ldap slave
 */
LDAPProtocol::LDAPProtocol( const QByteArray &protocol, const QByteArray &pool,
  const QByteArray &app ) : SlaveBase( protocol, pool, app )
{
  mLDAP = 0; mTLS = 0; mVer = 3; mAuthSASL = false;
  mRealm = ""; mBindName = "";
  mTimeLimit = mSizeLimit = 0;
  kDebug(7125) << "LDAPProtocol::LDAPProtocol (" << protocol << ")" << endl;
}

LDAPProtocol::~LDAPProtocol()
{
  closeConnection();
}

void LDAPProtocol::LDAPErr( const KUrl &url, int err )
{

  char *errmsg = 0;
  if ( mLDAP ) {
    if ( err == LDAP_SUCCESS ) ldap_get_option( mLDAP, LDAP_OPT_ERROR_NUMBER, &err );
    if ( err != LDAP_SUCCESS ) ldap_get_option( mLDAP, LDAP_OPT_ERROR_STRING, &errmsg );
  }
  if ( err == LDAP_SUCCESS ) return;
  kDebug(7125) << "error code: " << err << " msg: " << ldap_err2string(err) <<
    " Additonal error message: '" << errmsg << "'" << endl;
  QString msg;
  QString extraMsg;
  if ( errmsg ) {
    if ( errmsg[0] )
      extraMsg = i18n("\nAdditional info: ") + QString::fromUtf8( errmsg );
    free( errmsg );
  }
  msg = url.prettyUrl();
  if ( !extraMsg.isEmpty() ) msg += extraMsg;

  /* FIXME: No need to close on all errors */
  closeConnection();

  switch (err) {
/* FIXME: is it worth mapping the following error codes to kio errors?

	LDAP_OPERATIONS_ERROR
  LDAP_STRONG_AUTH_REQUIRED
	LDAP_PROTOCOL_ERROR
	LDAP_TIMELIMIT_EXCEEDED
	LDAP_SIZELIMIT_EXCEEDED
	LDAP_COMPARE_FALSE
	LDAP_COMPARE_TRUE
	LDAP_PARTIAL_RESULTS
	LDAP_NO_SUCH_ATTRIBUTE
	LDAP_UNDEFINED_TYPE
	LDAP_INAPPROPRIATE_MATCHING
	LDAP_CONSTRAINT_VIOLATION
	LDAP_INVALID_SYNTAX
	LDAP_NO_SUCH_OBJECT
	LDAP_ALIAS_PROBLEM
	LDAP_INVALID_DN_SYNTAX
	LDAP_IS_LEAF
	LDAP_ALIAS_DEREF_PROBLEM
	LDAP_INAPPROPRIATE_AUTH
	LDAP_BUSY
	LDAP_UNAVAILABLE
	LDAP_UNWILLING_TO_PERFORM
	LDAP_LOOP_DETECT
	LDAP_NAMING_VIOLATION
	LDAP_OBJECT_CLASS_VIOLATION
	LDAP_NOT_ALLOWED_ON_NONLEAF
	LDAP_NOT_ALLOWED_ON_RDN
	LDAP_NO_OBJECT_CLASS_MODS
	LDAP_OTHER
	LDAP_LOCAL_ERROR
	LDAP_ENCODING_ERROR
	LDAP_DECODING_ERROR
	LDAP_FILTER_ERROR
*/
    case LDAP_AUTH_UNKNOWN:
    case LDAP_INVALID_CREDENTIALS:
    case LDAP_STRONG_AUTH_NOT_SUPPORTED:
      error(ERR_COULD_NOT_AUTHENTICATE, msg);
      break;
    case LDAP_ALREADY_EXISTS:
      error(ERR_FILE_ALREADY_EXIST, msg);
      break;
    case LDAP_INSUFFICIENT_ACCESS:
      error(ERR_ACCESS_DENIED, msg);
      break;
    case LDAP_CONNECT_ERROR:
    case LDAP_SERVER_DOWN:
      error(ERR_COULD_NOT_CONNECT,msg);
      break;
    case LDAP_TIMEOUT:
      error(ERR_SERVER_TIMEOUT,msg);
      break;
    case LDAP_PARAM_ERROR:
      error(ERR_INTERNAL,msg);
      break;
    case LDAP_NO_MEMORY:
      error(ERR_OUT_OF_MEMORY,msg);
      break;

    default:
      error( ERR_SLAVE_DEFINED,
        i18n( "LDAP server returned the error: %1 %2\nThe LDAP URL was: %3" , 
         ldap_err2string(err), extraMsg, url.prettyUrl() ) );
  }
}

void LDAPProtocol::controlsFromMetaData( LDAPControl ***serverctrls,
  LDAPControl ***clientctrls )
{
  QString oid; bool critical; QByteArray value;
  int i = 0;
  while ( hasMetaData( QString::fromLatin1("SERVER_CTRL%1").arg(i) ) ) {
    QByteArray val = metaData( QString::fromLatin1("SERVER_CTRL%1").arg(i) ).toUtf8();
    LDIF::splitControl( val, oid, critical, value );
    kDebug(7125) << "server ctrl #" << i << " value: " << val <<
      " oid: " << oid << " critical: " << critical << " value: " <<
      QString::fromUtf8( value, value.size() ) << endl;
    addControlOp( serverctrls, oid, value, critical );
    i++;
  }
  i = 0;
  while ( hasMetaData( QString::fromLatin1("CLIENT_CTRL%1").arg(i) ) ) {
    QByteArray val = metaData( QString::fromLatin1("CLIENT_CTRL%1").arg(i) ).toUtf8();
    LDIF::splitControl( val, oid, critical, value );
    kDebug(7125) << "client ctrl #" << i << " value: " << val <<
      " oid: " << oid << " critical: " << critical << " value: " <<
      QString::fromUtf8( value, value.size() ) << endl;
    addControlOp( clientctrls, oid, value, critical );
    i++;
  }
}

int LDAPProtocol::asyncSearch( LDAPUrl &usrc, const QByteArray &cookie )
{
  char **attrs = 0;
  int msgid;
  LDAPControl **serverctrls = 0, **clientctrls = 0;

  int count = usrc.attributes().count();
  if ( count > 0 ) {
    attrs = static_cast<char**>( malloc((count+1) * sizeof(char*)) );
    for (int i=0; i<count; i++)
      attrs[i] = strdup( usrc.attributes().at(i).toUtf8() );
    attrs[count] = 0;
  }

  int retval, scope = LDAP_SCOPE_BASE;
  switch ( usrc.scope() ) {
    case LDAPUrl::Base:
      scope = LDAP_SCOPE_BASE;
      break;
    case LDAPUrl::One:
      scope = LDAP_SCOPE_ONELEVEL;
      break;
    case LDAPUrl::Sub:
      scope = LDAP_SCOPE_SUBTREE;
      break;
  }

  controlsFromMetaData( &serverctrls, &clientctrls );

  if ( mPageSize ) {
#ifdef LDAP_CONTROL_PAGEDRESULTS
    BerElement *prber;
    if (( prber = ber_alloc_t(LBER_USE_DER)) == NULL ) {
      if (attrs)
	 free(attrs);
      return -1;
    }
    struct berval c;
    c.bv_len = cookie.size();
    c.bv_val = (char *)cookie.data();
    ber_printf( prber, "{iO}", mPageSize, &c );
    struct berval bv;
    ber_flatten2( prber, &bv, 1 );
    addControlOp( &serverctrls, LDAP_CONTROL_PAGEDRESULTS, QByteArray::fromRawData(bv.bv_val,bv.bv_len), false );
    ber_memfree( bv.bv_val );
    ber_free( prber, 1 );
#else
    return -1;
#endif
  }
		       

  kDebug(7125) << "asyncSearch() dn=\"" << usrc.dn() << "\" scope=" <<
    usrc.scope() << " filter=\"" << usrc.filter() << "\" attrs=" << usrc.attributes() <<
    endl;
  retval = ldap_search_ext( mLDAP, usrc.dn().toUtf8(), scope,
    usrc.filter().isEmpty() ? QByteArray("objectClass=*") : usrc.filter().toUtf8(), 
    attrs, 0, serverctrls, clientctrls, 0, mSizeLimit, &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  // free the attributes list again
  if ( count > 0 ) {
    for ( int i=0; i<count; i++ ) free( attrs[i] );
    free(attrs);
  }

  if ( retval == 0 ) retval = msgid;
  return retval;
}

QByteArray LDAPProtocol::LDAPEntryAsLDIF( LDAPMessage *message )
{
  QByteArray result;
  char *name;
  struct berval **bvals;
  BerElement     *entry;

  char *dn = ldap_get_dn( mLDAP, message );
  if ( dn == NULL ) return QByteArray( "" );
  result += LDIF::assembleLine( "dn", QByteArray::fromRawData( dn, qstrlen( dn ) ) ) + '\n';
  ldap_memfree( dn );

  // iterate over the attributes
  name = ldap_first_attribute(mLDAP, message, &entry);
  while ( name != 0 )
  {
    // print the values
    bvals = ldap_get_values_len(mLDAP, message, name);
    if ( bvals ) {

      for ( int i = 0; bvals[i] != 0; i++ ) {
        char* val = bvals[i]->bv_val;
        unsigned long len = bvals[i]->bv_len;
        result += LDIF::assembleLine( QString::fromUtf8( name ), QByteArray::fromRawData( val, len ), 76 ) + '\n';
      }
      ldap_value_free_len(bvals);
    }
    // next attribute
    name = ldap_next_attribute(mLDAP, message, entry);
  }
  return result;
}

int LDAPProtocol::parsePageControl( LDAPMessage *result, QByteArray &cookie )
{
  int rc;
  int err;
  LDAPControl **ctrl = NULL;
  LDAPControl *ctrlp = NULL;
  BerElement *ber;
  ber_tag_t tag;

  rc = ldap_parse_result( mLDAP, result, &err, NULL, NULL, NULL, &ctrl, 0 );

  if( rc != LDAP_SUCCESS ) {
    kDebug(7125) << "ldap_parse_result" << endl;
    return -1;
  }

  if ( err != LDAP_SUCCESS ) {
    kDebug(7125) << "parse_page_control error: " << ldap_err2string(err) << " " << err << endl;
    return -1;
  }

  if( ctrl ) {
    /* Parse the control value
     * searchResult ::= SEQUENCE {
     *		size	INTEGER (0..maxInt),
     *				-- result set size estimate from server - unused
     *		cookie	OCTET STRING
     * }
     */
    ctrlp = *ctrl;
    ber = ber_init( &ctrlp->ldctl_value );
    if ( ber == 0 ) {
      kDebug(7125) << "Internal error." << endl;
      return -1;
    }

    int entriesLeft;
    struct berval bv;
    tag = ber_scanf( ber, "{io}", &entriesLeft, &bv );
    kDebug(7125) << "entriesleft: " << entriesLeft << " cookie len: " << bv.bv_len << " cookie: " << bv.bv_val << endl;
    (void) ber_free( ber, 1 );
    cookie.resize(bv.bv_len);
    memcpy(cookie.data(),bv.bv_val,bv.bv_len);
    ber_memfree(bv.bv_val);
		
    if( tag == LBER_ERROR ) {
      kDebug(7125) << "Paged results response control could not be decoded." << endl;
      return -1;
    }

    if( entriesLeft < 0 ) {
      kDebug(7125) << "Invalid entries estimate in paged results response." << endl;
      return -1;
    }

    ldap_controls_free( ctrl );

  }
  return err;
}

void LDAPProtocol::addControlOp( LDAPControl ***pctrls, const QString &oid,
  const QByteArray &value, bool critical )
{
  LDAPControl **ctrls;
  LDAPControl *ctrl = (LDAPControl *) malloc( sizeof( LDAPControl ) );

  ctrls = *pctrls;

  kDebug(7125) << "addControlOp: oid:'" << oid << "' val: '" << value << "'" << endl;
  int vallen = value.size();
  ctrl->ldctl_value.bv_len = vallen;
  if ( vallen ) {
    ctrl->ldctl_value.bv_val = (char*) malloc( vallen );
    memcpy( ctrl->ldctl_value.bv_val, value.data(), vallen );
  } else {
    ctrl->ldctl_value.bv_val = 0;
  }
  ctrl->ldctl_iscritical = critical;
  ctrl->ldctl_oid = strdup( oid.toUtf8() );

  uint i = 0;

  if ( ctrls == 0 ) {
    ctrls = (LDAPControl **) malloc ( 2 * sizeof( LDAPControl* ) );
    ctrls[ 0 ] = 0;
    ctrls[ 1 ] = 0;
  } else {
    while ( ctrls[ i ] != 0 ) i++;
    ctrls[ i + 1 ] = 0;
    ctrls = (LDAPControl **) realloc( ctrls, (i + 2) * sizeof( LDAPControl * ) );
  }
  ctrls[ i ] = ctrl;

  *pctrls = ctrls;
}

void LDAPProtocol::addModOp( LDAPMod ***pmods, int mod_type, const QString &attr,
  const QByteArray &value )
{
//  kDebug(7125) << "type: " << mod_type << " attr: " << attr <<
//    " value: " << QString::fromUtf8(value,value.size()) <<
//    " size: " << value.size() << endl;
  LDAPMod **mods;

  mods = *pmods;

  uint i = 0;

  if ( mods == 0 ) {
    mods = (LDAPMod **) malloc ( 2 * sizeof( LDAPMod* ) );
    mods[ 0 ] = (LDAPMod*) malloc( sizeof( LDAPMod ) );
    mods[ 1 ] = 0;
    memset( mods[ 0 ], 0, sizeof( LDAPMod ) );
  } else {
    while( mods[ i ] != 0 &&
      ( strcmp( attr.toUtf8(),mods[i]->mod_type ) != 0 ||
      ( mods[ i ]->mod_op & ~LDAP_MOD_BVALUES ) != mod_type ) ) i++;

    if ( mods[ i ] == 0 ) {
      mods = ( LDAPMod ** )realloc( mods, (i + 2) * sizeof( LDAPMod * ) );
      if ( mods == 0 ) {
        kError() << "addModOp: realloc" << endl;
        return;
      }
      mods[ i + 1 ] = 0;
      mods[ i ] = ( LDAPMod* ) malloc( sizeof( LDAPMod ) );
      memset( mods[ i ], 0, sizeof( LDAPMod ) );
    }
  }

  mods[ i ]->mod_op = mod_type | LDAP_MOD_BVALUES;
  if ( mods[ i ]->mod_type == 0 ) mods[ i ]->mod_type = strdup( attr.toUtf8() );

  *pmods = mods;

  int vallen = value.size();
  if ( vallen == 0 ) return;
  BerValue *berval;
  berval = ( BerValue* ) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, value.data(), vallen );

  if ( mods[ i ] -> mod_vals.modv_bvals == 0 ) {
    mods[ i ]->mod_vals.modv_bvals = ( BerValue** ) malloc( sizeof( BerValue* ) * 2 );
    mods[ i ]->mod_vals.modv_bvals[ 0 ] = berval;
    mods[ i ]->mod_vals.modv_bvals[ 1 ] = 0;
    kDebug(7125) << "addModOp: new bervalue struct " << endl;
  } else {
    uint j = 0;
    while ( mods[ i ]->mod_vals.modv_bvals[ j ] != 0 ) j++;
    mods[ i ]->mod_vals.modv_bvals = ( BerValue ** )
      realloc( mods[ i ]->mod_vals.modv_bvals, (j + 2) * sizeof( BerValue* ) );
    if ( mods[ i ]->mod_vals.modv_bvals == 0 ) {
      kError() << "addModOp: realloc" << endl;
      free(berval);
      return;
    }
    mods[ i ]->mod_vals.modv_bvals[ j ] = berval;
    mods[ i ]->mod_vals.modv_bvals[ j+1 ] = 0;
    kDebug(7125) << j << ". new bervalue " << endl;
  }
}

void LDAPProtocol::LDAPEntry2UDSEntry( const QString &dn, UDSEntry &entry,
  const LDAPUrl &usrc, bool dir )
{
  int pos;
  entry.clear();
  QString name = dn;
  if ( (pos = name.indexOf(',')) > 0 )
    name = name.left( pos );
  if ( (pos = name.indexOf('=')) > 0 )
    name.remove( 0, pos+1 );
  name.replace(' ', "_");
  if ( !dir ) name += ".ldif";
  entry.insert( UDS_NAME, name );

  // the file type
  entry.insert( UDS_FILE_TYPE, dir ? S_IFDIR : S_IFREG );

  // the mimetype
  if (!dir) {
    entry.insert( UDS_MIME_TYPE, QString::fromLatin1("text/plain") );
  }

  entry.insert( UDS_ACCESS, dir ? 0500 : 0400 );

  // the url
  LDAPUrl url=usrc;
  url.setPath('/'+dn);
  url.setScope( dir ? LDAPUrl::One : LDAPUrl::Base );
  entry.insert( UDS_URL, url.prettyUrl() );
}


void LDAPProtocol::changeCheck( LDAPUrl &url )
{
  bool critical;
  bool tls = ( url.hasExtension( "x-tls" ) );
  int ver = 3;
  if ( url.hasExtension( "x-ver" ) )
    ver = url.extension( "x-ver", critical).toInt();
  bool authSASL = url.hasExtension( "x-sasl" );
  QString mech;
  if ( url.hasExtension( "x-mech" ) )
    mech = url.extension( "x-mech", critical).toUpper();
  QString realm;
  if ( url.hasExtension( "x-realm" ) )
    mech = url.extension( "x-realm", critical).toUpper();
  QString bindname;
  if ( url.hasExtension( "bindname" ) )
    bindname = url.extension( "bindname", critical).toUpper();
  int timelimit = 0;
  if ( url.hasExtension( "x-timelimit" ) )
    timelimit = url.extension( "x-timelimit", critical).toInt();
  int sizelimit = 0;
  if ( url.hasExtension( "x-sizelimit" ) )
    sizelimit = url.extension( "x-sizelimit", critical).toInt();
  if ( url.hasExtension( "x-pagesize" ) )
    mPageSize = url.extension( "x-pagesize", critical).toInt();
  else
    mPageSize = 0;

  if ( !authSASL && bindname.isEmpty() ) bindname = mUser;

  if ( tls != mTLS || ver != mVer || authSASL != mAuthSASL || mech != mMech ||
    mRealm != realm || mBindName != bindname || mTimeLimit != timelimit ||
    mSizeLimit != sizelimit ) {
    closeConnection();
    mTLS = tls;
    mVer = ver;
    mAuthSASL = authSASL;
    mMech = mech;
    mRealm = realm;
    mBindName = bindname;
    mTimeLimit = timelimit;
    mSizeLimit = sizelimit;
    kDebug(7125) << "parameters changed: tls = " << mTLS <<
      " version: " << mVer << "SASLauth: " << mAuthSASL << endl;
    openConnection();
    if ( mAuthSASL ) {
      url.setUser( mUser );
    } else {
      url.setUser( mBindName );
    }
  } else {
    if ( !mLDAP ) openConnection();
  }
}

void LDAPProtocol::setHost( const QString& host, int port,
                            const QString& user, const QString& password )
{

  if( mHost != host || mPort != port || mUser != user || mPassword != password )
    closeConnection();

  mHost = host;
  if( port > 0 )
    mPort = port;
  else {
    struct servent *pse;
    if ( (pse = getservbyname(mProtocol, "tcp")) == NULL )
      if ( mProtocol == "ldaps" )
        mPort = 636;
      else
        mPort = 389;
    else
      mPort = ntohs( pse->s_port );
  }
  mUser = user;
  mPassword = password;

  kDebug(7125) << "setHost: " << host << " port: " << port << " user: " <<
    mUser << " pass: [protected]" << endl;
}

static int kldap_sasl_interact( LDAP *, unsigned, void *slave, void *in )
{
  return ((LDAPProtocol*) slave)->saslInteract( in );
}

void LDAPProtocol::fillAuthInfo( AuthInfo &info )
{
  info.url.setProtocol( mProtocol );
  info.url.setHost( mHost );
  info.url.setPort( mPort );
  info.url.setUser( mUser );
  info.caption = i18n("LDAP Login");
  info.comment = QString::fromLatin1( mProtocol ) + "://" + mHost + ':' +
    QString::number( mPort );
  info.commentLabel = i18n("site:");
  info.username = mAuthSASL ? mUser : mBindName;
  info.password = mPassword;
  info.keepPassword = true;
}

int LDAPProtocol::saslInteract( void *in )
{
#if defined HAVE_SASL_H || defined HAVE_SASL_SASL_H
  AuthInfo info;
  fillAuthInfo( info );

  sasl_interact_t *interact = ( sasl_interact_t * ) in;

  //some mechanisms do not require username && pass, so it doesn't need a popup
  //window for getting this info
  for ( ; interact->id != SASL_CB_LIST_END; interact++ ) {
    if ( interact->id == SASL_CB_AUTHNAME ||
         interact->id == SASL_CB_PASS ) {

      if ( info.username.isEmpty() || info.password.isEmpty() ) {

        const bool cached = checkCachedAuthentication( info );

        if ( ! ( ( mFirstAuth && cached ) ||
                 ( mFirstAuth ?
                   openPassDlg( info ) :
                   openPassDlg( info, i18n("Invalid authorization information.") ) ) ) ) {
          kDebug(7125) << "Dialog canceled!" << endl;
          mCancel = true;
          return LDAP_USER_CANCELLED;
        }
        mUser = info.username;
        mPassword = info.password;
      }
      break;
    }
  }

  interact = ( sasl_interact_t * ) in;
  QString value;

  while( interact->id != SASL_CB_LIST_END ) {
    value = "";
    switch( interact->id ) {
      case SASL_CB_GETREALM:
        value = mRealm;
        kDebug(7125) << "SASL_REALM=" << mRealm << endl;
        break;
      case SASL_CB_AUTHNAME:
        value = mUser;
        kDebug(7125) << "SASL_AUTHNAME=" << mUser << endl;
        break;
      case SASL_CB_PASS:
        value = mPassword;
        kDebug(7125) << "SASL_PASSWD=[hidden]" << endl;
        break;
      case SASL_CB_USER:
        value = mBindName;
        kDebug(7125) << "SASL_AUTHZID=" << mBindName << endl;
        break;
    }
    if ( value.isEmpty() ) {
      interact->result = NULL;
      interact->len = 0;
    } else {
      interact->result = strdup( value.toUtf8() );
      interact->len = strlen( (const char *) interact->result );
    }
    interact++;
  }

#endif
  return LDAP_SUCCESS;
}

void LDAPProtocol::openConnection()
{
  if ( mLDAP ) return;

  int version,ret;

  version = ( mVer == 2 ) ? LDAP_VERSION2 : LDAP_VERSION3;

  KUrl Url;
  Url.setProtocol( mProtocol );
  Url.setHost( mHost );
  Url.setPort( mPort );

  AuthInfo info;
  fillAuthInfo( info );
///////////////////////////////////////////////////////////////////////////
  kDebug(7125) << "OpenConnection to " << mHost << ":" << mPort << endl;

  ret = ldap_initialize( &mLDAP, Url.url().toLatin1() );
  if ( ret != LDAP_SUCCESS ) {
    LDAPErr( Url, ret );
    return;
  }

  if ( (ldap_set_option( mLDAP, LDAP_OPT_PROTOCOL_VERSION, &version )) !=
    LDAP_OPT_SUCCESS ) {

    closeConnection();
    error( ERR_UNSUPPORTED_ACTION,
      i18n("Cannot set LDAP protocol version %1", version) );
    return;
  }

  if ( mTLS ) {
    kDebug(7125) << "start TLS" << endl;
    if ( ( ret = ldap_start_tls_s( mLDAP, NULL, NULL ) ) != LDAP_SUCCESS ) {
      LDAPErr( Url );
      return;
    }
  }

  if ( mSizeLimit ) {
    kDebug(7125) << "sizelimit: " << mSizeLimit << endl;
    if ( ldap_set_option( mLDAP, LDAP_OPT_SIZELIMIT, &mSizeLimit ) != LDAP_SUCCESS ) {
      closeConnection();
      error( ERR_UNSUPPORTED_ACTION,
        i18n("Cannot set size limit."));
      return;
    }
  }

  if ( mTimeLimit ) {
    kDebug(7125) << "timelimit: " << mTimeLimit << endl;
    if ( ldap_set_option( mLDAP, LDAP_OPT_TIMELIMIT, &mTimeLimit ) != LDAP_SUCCESS ) {
      closeConnection();
      error( ERR_UNSUPPORTED_ACTION,
        i18n("Cannot set time limit."));
      return;
    }
  }

#if !defined HAVE_SASL_H && !defined HAVE_SASL_SASL_H
  if ( mAuthSASL ) {
    closeConnection();
    error( ERR_SLAVE_DEFINED,
      i18n("SASL authentication not compiled into the ldap ioslave.") );
    return;
  }
#endif

  bool auth = false;
  QString mechanism = mMech.isEmpty() ? "DIGEST-MD5" : mMech;
  mFirstAuth = true; mCancel = false;

  const bool cached = checkCachedAuthentication( info );

  ret = LDAP_SUCCESS;
  while (!auth) {
    if ( !mAuthSASL && (
      ( mFirstAuth &&
      !( mBindName.isEmpty() && mPassword.isEmpty() ) && //For anonymous bind
       ( mBindName.isEmpty() || mPassword.isEmpty() ) ) || !mFirstAuth ) )
    {
      if ( ( mFirstAuth && cached ) ||
           ( mFirstAuth ?
             openPassDlg( info ) :
             openPassDlg( info, i18n("Invalid authorization information.") ) ) ) {

        mBindName = info.username;
        mPassword = info.password;
      } else {
        kDebug(7125) << "Dialog canceled!" << endl;
        error( ERR_USER_CANCELED, QString() );
        closeConnection();
        return;
      }
    }
    kDebug(7125) << "user: " << mUser << " bindname: " << mBindName << endl;
    ret =
#if defined HAVE_SASL_H || defined HAVE_SASL_SASL_H
      mAuthSASL ?
        ldap_sasl_interactive_bind_s( mLDAP, NULL, mechanism.toUtf8(),
          NULL, NULL, LDAP_SASL_INTERACTIVE, &kldap_sasl_interact, this ) :
#endif
        ldap_simple_bind_s( mLDAP, mBindName.toUtf8(), mPassword.toUtf8() );

    mFirstAuth = false;
    if ( ret != LDAP_INVALID_CREDENTIALS &&
         ret != LDAP_INSUFFICIENT_ACCESS &&
         ret != LDAP_INAPPROPRIATE_AUTH ) {
      kDebug(7125) << "ldap_bind retval: " << ret << endl;
      auth = true;
      if ( ret != LDAP_SUCCESS ) {
        if ( mCancel )
          error( ERR_USER_CANCELED, QString() );
        else
          LDAPErr( Url );
        closeConnection();
        return;
      }
    }
  }

  kDebug(7125) << "connected!" << endl;
  connected();
}

void LDAPProtocol::closeConnection()
{
  if (mLDAP) ldap_unbind(mLDAP);
  mLDAP = 0;
  kDebug(7125) << "connection closed!" << endl;
}

/**
 * Get the information contained in the URL.
 */
void LDAPProtocol::get( const KUrl &_url )
{
  kDebug(7125) << "get(" << _url << ")" << endl;

  LDAPUrl usrc(_url);
  int ret, id;
  LDAPMessage *msg,*entry;

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }

  if ( (id = asyncSearch( usrc )) == -1 ) {
    LDAPErr( _url );
    return;
  }

  // tell the mimetype
  mimeType("text/plain");
  // collect the result
  QByteArray result;
  filesize_t processed_size = 0;

  while( true ) {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      LDAPErr( _url );
      return;
    }
    kDebug(7125) << " ldap_result: " << ret << endl;
    if ( ret == LDAP_RES_SEARCH_RESULT ) {
      if ( mPageSize ) {
        QByteArray cookie;
        if ( parsePageControl( msg, cookie ) != LDAP_SUCCESS ) {
          LDAPErr( _url );
          return;
        }
        if ( !cookie.isEmpty() ) {
          if ( (id = asyncSearch( usrc, cookie )) == -1 ) {
            LDAPErr( _url );
            return;
          }
	  continue;
        }
      }
      break;
    }
    if ( ret != LDAP_RES_SEARCH_ENTRY ) continue;

    entry = ldap_first_entry( mLDAP, msg );
    while ( entry ) {
      result = LDAPEntryAsLDIF(entry);
      result += '\n';
      uint len = result.length();
      processed_size += len;
      data(result);
      processedSize( processed_size );

      entry = ldap_next_entry( mLDAP, entry );
    }
    LDAPErr( _url );

    ldap_msgfree(msg);
  // tell the length
  }

  totalSize(processed_size);

  // tell we are finished
  data( QByteArray() );

  // tell we are finished
  finished();
}

/**
 * Test if the url contains a directory or a file.
 */
void LDAPProtocol::stat( const KUrl &_url )
{
  kDebug(7125) << "stat(" << _url << ")" << endl;

  QStringList att,saveatt;
  LDAPUrl usrc(_url);
  LDAPMessage *msg;
  int ret, id;

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }

  // look how many entries match
  saveatt = usrc.attributes();
  att.append( "dn" );
  usrc.setAttributes( att );
  if ( _url.query().isEmpty() ) usrc.setScope( LDAPUrl::One );

  if ( (id = asyncSearch( usrc )) == -1 ) {
    LDAPErr( _url );
    return;
  }

  kDebug(7125) << "stat() getting result" << endl;
  do {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      LDAPErr( _url );
      return;
    }
    if ( ret == LDAP_RES_SEARCH_RESULT ) {
      ldap_msgfree( msg );
      error( ERR_DOES_NOT_EXIST, _url.prettyUrl() );
      return;
    }
  } while ( ret != LDAP_RES_SEARCH_ENTRY );

  ldap_msgfree( msg );
  ldap_abandon( mLDAP, id );

  usrc.setAttributes( saveatt );

  UDSEntry uds;
  bool critical;
  LDAPEntry2UDSEntry( usrc.dn(), uds, usrc, usrc.extension("x-dir", critical) != "base" );

  statEntry( uds );
  // we are done
  finished();
}

/**
 * Deletes one entry;
 */
void LDAPProtocol::del( const KUrl &_url, bool )
{
  kDebug(7125) << "del(" << _url << ")" << endl;

  LDAPUrl usrc(_url);
  int ret;

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }

  kDebug(7125) << " del: " << usrc.dn().toUtf8() << endl ;

  if ( (ret = ldap_delete_s( mLDAP,usrc.dn().toUtf8() )) != LDAP_SUCCESS ) {
    LDAPErr( _url );
    return;
  }
  finished();
}

#define FREELDAPMEM { \
                ldap_mods_free( lmod, 1 ); \
                ldap_controls_free( serverctrls ); \
                ldap_controls_free( clientctrls ); \
                lmod = 0; serverctrls = 0; clientctrls = 0; \
                }

void LDAPProtocol::put( const KUrl &_url, int, bool overwrite, bool )
{
  kDebug(7125) << "put(" << _url << ")" << endl;

  LDAPUrl usrc(_url);

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }

  LDAPMod **lmod = 0;
  LDAPControl **serverctrls = 0, **clientctrls = 0;
  QByteArray buffer;
  int result = 0;
  LDIF::ParseValue ret;
  LDIF ldif;
  ret = LDIF::MoreData;
  int ldaperr;


  do {
    if ( ret == LDIF::MoreData ) {
      dataReq(); // Request for data
      result = readData( buffer );
      ldif.setLDIF( buffer );
    }
    if ( result < 0 ) {
      //error
      FREELDAPMEM;
      return;
    }
    if ( result == 0 ) {
      kDebug(7125) << "EOF!" << endl;
      ldif.endLDIF();
    }
    do {

      ret = ldif.nextItem();
      kDebug(7125) << "nextitem: " << ret << endl;

      switch ( ret ) {
        case LDIF::None:
        case LDIF::NewEntry:
        case LDIF::MoreData:
          break;
        case LDIF::EndEntry:
          ldaperr = LDAP_SUCCESS;
          switch ( ldif.entryType() ) {
            case LDIF::Entry_None:
              error( ERR_INTERNAL, i18n("The LDIF parser failed.") );
              FREELDAPMEM;
              return;
            case LDIF::Entry_Del:
              kDebug(7125) << "kio_ldap_del" << endl;
              controlsFromMetaData( &serverctrls, &clientctrls );
              ldaperr = ldap_delete_ext_s( mLDAP, ldif.dn().toUtf8(),
                serverctrls, clientctrls );
              FREELDAPMEM;
              break;
            case LDIF::Entry_Modrdn:
              kDebug(7125) << "kio_ldap_modrdn olddn:" << ldif.dn() <<
                " newRdn: " <<  ldif.newRdn() <<
                " newSuperior: " << ldif.newSuperior() <<
                " deloldrdn: " << ldif.delOldRdn() << endl;
              controlsFromMetaData( &serverctrls, &clientctrls );
              ldaperr = ldap_rename_s( mLDAP, ldif.dn().toUtf8(), ldif.newRdn().toUtf8(),
                ldif.newSuperior().isEmpty() ? QByteArray() : ldif.newSuperior().toUtf8(),
                ldif.delOldRdn(), serverctrls, clientctrls );

              FREELDAPMEM;
              break;
            case LDIF::Entry_Mod:
              kDebug(7125) << "kio_ldap_mod"  << endl;
              if ( lmod ) {
                controlsFromMetaData( &serverctrls, &clientctrls );
                ldaperr = ldap_modify_ext_s( mLDAP, ldif.dn().toUtf8(), lmod,
                  serverctrls, clientctrls );
                FREELDAPMEM;
              }
              break;
            case LDIF::Entry_Add:
              kDebug(7125) << "kio_ldap_add " << ldif.dn() << endl;
              if ( lmod ) {
                controlsFromMetaData( &serverctrls, &clientctrls );
                ldaperr = ldap_add_ext_s( mLDAP, ldif.dn().toUtf8(), lmod,
                  serverctrls, clientctrls );
                if ( ldaperr == LDAP_ALREADY_EXISTS && overwrite ) {
                  kDebug(7125) << ldif.dn() << " already exists, delete first" << endl;
                  ldaperr = ldap_delete_s( mLDAP, ldif.dn().toUtf8() );
                  if ( ldaperr == LDAP_SUCCESS )
                    ldaperr = ldap_add_ext_s( mLDAP, ldif.dn().toUtf8(), lmod,
                      serverctrls, clientctrls );
                }
                FREELDAPMEM;
              }
              break;
          }
          if ( ldaperr != LDAP_SUCCESS ) {
            kDebug(7125) << "put ldap error: " << ldap_err2string(ldaperr) << endl;
            LDAPErr( _url );
            FREELDAPMEM;
            return;
          }
          break;
        case LDIF::Item:
          switch ( ldif.entryType() ) {
            case LDIF::Entry_Mod: {
              int modtype = 0;
              switch ( ldif.modType() ) {
                case LDIF::Mod_None:
                  modtype = 0;
                  break;
                case LDIF::Mod_Add:
                  modtype = LDAP_MOD_ADD;
                  break;
                case LDIF::Mod_Replace:
                  modtype = LDAP_MOD_REPLACE;
                  break;
                case LDIF::Mod_Del:
                  modtype = LDAP_MOD_DELETE;
                  break;
              }
              addModOp( &lmod, modtype, ldif.attr(), ldif.value() );
              break;
            }
            case LDIF::Entry_Add:
              if ( ldif.value().size() > 0 )
                addModOp( &lmod, 0, ldif.attr(), ldif.value() );
              break;
            default:
              error( ERR_INTERNAL, i18n("The LDIF parser failed.") );
              FREELDAPMEM;
              return;
          }
          break;
        case LDIF::Control:
          addControlOp( &serverctrls, ldif.oid(), ldif.value(), ldif.isCritical() );
          break;
        case LDIF::Err:
          error( ERR_SLAVE_DEFINED,
            i18n( "Invalid LDIF file in line %1.", ldif.lineNumber() ) );
          FREELDAPMEM;
          return;
      }
    } while ( ret != LDIF::MoreData );
  } while ( result > 0 );

  FREELDAPMEM;
  finished();
}

/**
 * List the contents of a directory.
 */
void LDAPProtocol::listDir( const KUrl &_url )
{
  int ret, ret2, id, id2;
  unsigned long total=0;
  char *dn;
  QStringList att,saveatt;
  LDAPMessage *entry,*msg,*entry2,*msg2;
  LDAPUrl usrc(_url),usrc2;
  bool critical;
  bool isSub = ( usrc.extension( "x-dir", critical ) == "sub" );

  kDebug(7125) << "listDir(" << _url << ")" << endl;

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }
  usrc2 = usrc;

  saveatt = usrc.attributes();
  // look up the entries
  if ( isSub ) {
    att.append("dn");
    usrc.setAttributes(att);
  }
  if ( _url.query().isEmpty() ) usrc.setScope( LDAPUrl::One );

  if ( (id = asyncSearch( usrc )) == -1 ) {
    LDAPErr( _url );
    return;
  }

  usrc.setAttributes( QStringList() << "" );
  usrc.setExtension( "x-dir", "base" );
  // publish the results
  UDSEntry uds;

  while( true ) {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      LDAPErr( _url );
      return;
    }
    if ( ret == LDAP_RES_SEARCH_RESULT ) break;
    if ( ret != LDAP_RES_SEARCH_ENTRY ) continue;
    kDebug(7125) << " ldap_result: " << ret << endl;

    entry = ldap_first_entry( mLDAP, msg );
    while( entry ) {

      total++;
      uds.clear();

      dn = ldap_get_dn( mLDAP, entry );
      kDebug(7125) << "dn: " << dn  << endl;
      LDAPEntry2UDSEntry( QString::fromUtf8(dn), uds, usrc );
      listEntry( uds, false );
//      processedSize( total );
      kDebug(7125) << " total: " << total << " " << usrc.prettyUrl() << endl;

    // publish the sub-directories (if dirmode==sub)
      if ( isSub ) {
        usrc2.setDn( QString::fromUtf8( dn ) );
        usrc2.setScope( LDAPUrl::One );
        usrc2.setAttributes( att );
        usrc2.setFilter( QString() );
        kDebug(7125) << "search2 " << dn << endl;
        if ( (id2 = asyncSearch( usrc2 )) != -1 ) {
          while ( true ) {
            kDebug(7125) << " next result " << endl;
            ret2 = ldap_result( mLDAP, id2, 0, NULL, &msg2 );
            if ( ret2 == -1 ) break;
            if ( ret2 == LDAP_RES_SEARCH_RESULT ) {
              ldap_msgfree( msg2 );
              break;
            }
            if ( ret2 == LDAP_RES_SEARCH_ENTRY ) {
              entry2=ldap_first_entry( mLDAP, msg2 );
              if  ( entry2 ) {
                usrc2.setAttributes( saveatt );
                usrc2.setFilter( usrc.filter() );
                LDAPEntry2UDSEntry( QString::fromUtf8( dn ), uds, usrc2, true );
                listEntry( uds, false );
                total++;
              }
              ldap_msgfree( msg2 );
              ldap_abandon( mLDAP, id2 );
              break;
            }
          }
        }
      }
      free( dn );

      entry = ldap_next_entry( mLDAP, entry );
    }
    LDAPErr( _url );
    ldap_msgfree( msg );
  }

//  totalSize( total );

  uds.clear();
  listEntry( uds, true );
  // we are done
  finished();
}
