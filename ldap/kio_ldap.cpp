
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>

#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>

#ifdef HAVE_SASL_H
#include <sasl.h>
#endif
#include <kabc/ldif.h>

#include "kio_ldap.h"

using namespace KIO;
using namespace KABC;

extern "C" { int kdemain(int argc, char **argv); }

/**
 * The main program.
 */
int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_ldap" );

  kdDebug(7125) << "Starting " << getpid() << endl;

  if ( argc != 4 ) {
    kdError() << "Usage kio_ldap protocol pool app" << endl;
    return -1;
  }

  // let the protocol class do its work
  LDAPProtocol slave( argv[1], argv[ 2 ], argv[ 3 ] );
  slave.dispatchLoop();

  kdDebug( 7125 ) << "Done" << endl;
  return 0;
}

/**
 * Initialize the ldap slave
 */
LDAPProtocol::LDAPProtocol( const QCString &protocol, const QCString &pool, 
  const QCString &app ) : SlaveBase( protocol, pool, app )
{
  mLDAP = 0; mTLS = 0; mVer = 3; mAuthSASL = false;
  mRealm = ""; mBindName = "";
  mTimeLimit = mSizeLimit = 0;
  kdDebug(7125) << "LDAPProtocol::LDAPProtocol (" << protocol << ")" << endl;
}

LDAPProtocol::~LDAPProtocol() 
{
  closeConnection();
}

void LDAPProtocol::checkErr( const KURL &_url )
{
  int ret;
  
  if ( ldap_get_option( mLDAP, LDAP_OPT_ERROR_NUMBER, &ret ) == -1 ) {
    error( KIO::ERR_UNKNOWN, _url.prettyURL() );
  } else {
    if ( ret != LDAP_SUCCESS ) LDAPErr( ret, _url.prettyURL() );
  }
}

void LDAPProtocol::LDAPErr( int err, const QString &msg )
{
  kdDebug(7125) << "error: " << err << " " << ldap_err2string(err) << endl;
  
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
        i18n( "LDAP server returned the following error: %1" ). 
        arg( ldap_err2string(err)) );
  }
}

int LDAPProtocol::asyncSearch( LDAPUrl &usrc ) 
{
  char **attrs = 0;
  int count = usrc.attributes().count();
  if ( count > 0 ) {
    attrs = static_cast<char**>( malloc((count+1) * sizeof(char*)) );
    for (int i=0; i<count; i++)
      attrs[i] = strdup( (*usrc.attributes().at(i)).utf8() );
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

  kdDebug(7125) << "asyncSearch() dn=" << usrc.dn() << " scope=" << 
    usrc.scope() << " filter=" << usrc.filter() << "attrs=" << usrc.attributes() << 
    endl;
  retval = ldap_search( mLDAP, usrc.dn().utf8(), scope, 
    usrc.filter().isEmpty() ? 0 : usrc.filter().utf8(), attrs, 0 );

  // free the attributes list again
  if ( count > 0 ) {
    for ( int i=0; i<count; i++ ) free( attrs[i] );
    free(attrs);
  }
  
  return retval;
}

QCString LDAPProtocol::LDAPEntryAsLDIF( LDAPMessage *message )
{
  QCString result;
  char *name;
  struct berval **bvals;
  BerElement     *entry;
  QByteArray tmp;
  
  char *dn = ldap_get_dn( mLDAP, message );
  if ( dn == NULL ) return QCString( "" );
  tmp.setRawData( dn, strlen( dn ) );
  result += LDIF::assembleLine( "dn", tmp ) + '\n';
  tmp.resetRawData( dn, strlen( dn ) );
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
        tmp.setRawData( val, len );
        result += LDIF::assembleLine( QString::fromUtf8( name ), tmp, 76 ) + '\n';
        tmp.resetRawData( val, len );
      }
      ldap_value_free_len(bvals);
    }
    // next attribute
    name = ldap_next_attribute(mLDAP, message, entry);
  }
  return result;
}

void LDAPProtocol::addControlOp( LDAPControl ***pctrls, const QString &oid,
  const QByteArray &value, bool critical )
{
  LDAPControl **ctrls;
  LDAPControl *ctrl = (LDAPControl *) malloc( sizeof( LDAPControl ) );
    
  ctrls = *pctrls;

  kdDebug(7125) << "addControlOp: oid:'" << oid << "' val: '" << 
    QString::fromUtf8(value, value.size()) << "'" << endl;
  int vallen = value.size();
  ctrl->ldctl_value.bv_len = vallen;
  if ( vallen ) {
    ctrl->ldctl_value.bv_val = (char*) malloc( vallen );
    memcpy( ctrl->ldctl_value.bv_val, value.data(), vallen );
  } else {
    ctrl->ldctl_value.bv_val = 0;
  }
  ctrl->ldctl_iscritical = critical;
  ctrl->ldctl_oid = strdup( oid.utf8() );
  
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
//  kdDebug(7125) << "type: " << mod_type << " attr: " << attr << 
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
      ( strcmp( attr.utf8(),mods[i]->mod_type ) != 0 ||
      ( mods[ i ]->mod_op & ~LDAP_MOD_BVALUES ) != mod_type ) ) i++;
    
    if ( mods[ i ] == 0 ) {
      mods = ( LDAPMod ** )realloc( mods, (i + 2) * sizeof( LDAPMod * ) );
      if ( mods == 0 ) {
        kdError() << "addModOp: realloc" << endl;
        return;
      }
      mods[ i + 1 ] = 0;
      mods[ i ] = ( LDAPMod* ) malloc( sizeof( LDAPMod ) );
      memset( mods[ i ], 0, sizeof( LDAPMod ) );
    }
  }

  mods[ i ]->mod_op = mod_type | LDAP_MOD_BVALUES;
  if ( mods[ i ]->mod_type == 0 ) mods[ i ]->mod_type = strdup( attr.utf8() );
  
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
    kdDebug(7125) << "addModOp: new bervalue struct " << endl;
  } else {
    uint j = 0;
    while ( mods[ i ]->mod_vals.modv_bvals[ j ] != 0 ) j++;
    mods[ i ]->mod_vals.modv_bvals = ( BerValue ** ) 
      realloc( mods[ i ]->mod_vals.modv_bvals, (j + 2) * sizeof( BerValue* ) );
    if ( mods[ i ]->mod_vals.modv_bvals == 0 ) {
      kdError() << "addModOp: realloc" << endl;
      return;
    }
    mods[ i ]->mod_vals.modv_bvals[ j ] = berval;     
    mods[ i ]->mod_vals.modv_bvals[ j+1 ] = 0;     
    kdDebug(7125) << j << ". new bervalue " << endl;
  }
}

void LDAPProtocol::LDAPEntry2UDSEntry( const QString &dn, UDSEntry &entry, 
  const LDAPUrl &usrc, bool dir )
{
  UDSAtom atom;
  
  int pos;
  entry.clear();
  atom.m_uds = UDS_NAME;
  atom.m_long = 0;
  QString name = dn;
  if ( (pos = name.find(",")) > 0 )
    name = name.left( pos );
  if ( (pos = name.find("=")) > 0 )
    name.remove( 0, pos+1 );
  name.replace(' ', "_");
  if ( !dir ) name += ".ldif";
  atom.m_str = name;
  entry.append( atom );

  // the file type
  atom.m_uds = UDS_FILE_TYPE;
  atom.m_str = "";
  atom.m_long = dir ? S_IFDIR : S_IFREG;
  entry.append( atom );
  
  // the mimetype
  if (!dir) {
    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/plain";
    entry.append( atom );
  }

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = dir ? 0500 : 0400;
  entry.append( atom );

  // the url
  atom.m_uds = UDS_URL;
  atom.m_long = 0;
  LDAPUrl url;
  url=usrc;

  url.setPath("/"+dn);
  url.setScope( dir ? LDAPUrl::One : LDAPUrl::Base );
  atom.m_str = url.prettyURL();
  entry.append( atom );
}

void LDAPProtocol::changeCheck( const LDAPUrl &url )
{
  bool critical;
  bool tls = ( url.hasExtension( "x-tls" ) );
  int ver = 3;
  if ( url.hasExtension( "x-ver" ) ) 
    ver = url.extension( "x-ver", critical).toInt();
  bool authSASL = url.hasExtension( "x-sasl" );
  QString mech;
  if ( url.hasExtension( "x-mech" ) ) 
    mech = url.extension( "x-mech", critical).upper();
  QString realm;
  if ( url.hasExtension( "x-realm" ) ) 
    mech = url.extension( "x-realm", critical).upper();
  QString bindname;
  if ( url.hasExtension( "bindname" ) ) 
    bindname = url.extension( "bindname", critical).upper();
  int timelimit = 0;
  if ( url.hasExtension( "x-timelimit" ) )
    timelimit = url.extension( "x-timelimit", critical).toInt();
  int sizelimit = 0;
  if ( url.hasExtension( "x-sizelimit" ) )
    sizelimit = url.extension( "x-sizelimit", critical).toInt();
    
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
    kdDebug(7125) << "parameters changed: tls = " << mTLS << 
      " version: " << mVer << "SASLauth: " << mAuthSASL << endl;
    openConnection();
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

  kdDebug(7125) << "setHost: " << host << " port: " << port << " user: " << 
    mUser << " pass: [protected]" << endl;
}
    
typedef struct kldap_sasl_defaults_t {
  QString realm;
  QString authcid;
  QString passwd;
  QString authzid;
} kldap_sasl_defaults;

#ifdef HAVE_SASL_H
static int kldap_sasl_interact( LDAP *, unsigned, void *defaults, void *in )
{
  sasl_interact_t *interact = ( sasl_interact_t * ) in;
  kldap_sasl_defaults *def = ( kldap_sasl_defaults * ) defaults;  
  QString value;
  
  while( interact->id != SASL_CB_LIST_END ) {
    value = "";
    switch( interact->id ) {
      case SASL_CB_GETREALM:
        value = def->realm;
        kdDebug(7125) << "SASL_REALM=" << value << endl;
        break;
      case SASL_CB_AUTHNAME:
        value = def->authcid;
        kdDebug(7125) << "SASL_AUTHNAME=" << value << endl;
        break;
      case SASL_CB_PASS:
        value = def->passwd;
        kdDebug(7125) << "SASL_PASSWD=[hidden]" << endl;
        break;
      case SASL_CB_USER:
        value = def->authzid;
        kdDebug(7125) << "SASL_AUTHZID=" << value << endl;
        break;
    }
    if ( value.isEmpty() ) {
      interact->result = NULL;
      interact->len = 0;
    } else {
      interact->result = strdup( value.utf8() );
      interact->len = strlen( (const char *) interact->result );
    }
    interact++;
  }

  return LDAP_SUCCESS;
}
#endif

void LDAPProtocol::openConnection()
{
  if ( mLDAP ) return;
  
  int version,ret;
  
  version = ( mVer == 2 ) ? LDAP_VERSION2 : LDAP_VERSION3;
  
  KURL Url;
  Url.setProtocol( mProtocol );
  Url.setHost( mHost );
  Url.setPort( mPort );
   
  AuthInfo info;
  info.url.setProtocol( mProtocol );
  info.url.setHost( mHost );
  info.url.setPort( mPort );
  info.url.setUser( mUser );
  info.caption = i18n("LDAP Login");
  info.comment = QString::fromLatin1( mProtocol ) + "://" + mHost + ":" + 
    QString::number( mPort );
  info.commentLabel = i18n("site:");
  info.username = mAuthSASL ? mUser : mBindName;
  info.keepPassword = true;

  ///////////////////////////////////////////////////////////////////////////

  if( mUser.isEmpty() && mPassword.isEmpty() && 
//don't need authentication info for kerberos-gssapi
    !( mAuthSASL && mMech == "GSSAPI" ) ) {
    if( checkCachedAuthentication( info ) ) {
      if ( mAuthSASL )
        mUser = info.username;
      else
        mBindName = info.username;
      mPassword = info.password;
      kdDebug(7125) << "auth info from cache: " << mUser <<  endl;
    }
  }

  kdDebug(7125) << "OpenConnection to " << mHost << ":" << mPort << endl;

  ret = ldap_initialize( &mLDAP, Url.htmlURL().utf8() );
  if ( ret != LDAP_SUCCESS ) {
    LDAPErr( ret, Url.prettyURL() );
    closeConnection();
    return;
  }
  
  if ( (ldap_set_option( mLDAP, LDAP_OPT_PROTOCOL_VERSION, &version )) != 
    LDAP_OPT_SUCCESS ) {
    
    closeConnection();
    error( ERR_UNSUPPORTED_ACTION, 
      i18n("Cannot set LDAP protocol version %1").arg(version) );
    return;
  }
  
  if ( mTLS ) {
    kdDebug(7125) << "start TLS" << endl;
    if ( ( ret = ldap_start_tls_s( mLDAP, NULL, NULL ) ) != LDAP_SUCCESS ) {
      closeConnection();
      LDAPErr( ret, Url.prettyURL() );
      return;
    }
  }
  
  if ( mSizeLimit ) {
    kdDebug(7125) << "sizelimit: " << mSizeLimit << endl;
    if ( ldap_set_option( mLDAP, LDAP_OPT_SIZELIMIT, &mSizeLimit ) != LDAP_SUCCESS ) {
      closeConnection();
      error( ERR_UNSUPPORTED_ACTION, 
        i18n("Cannot set size limit.").arg(version) );
      return;
    }
  }
  
  if ( mTimeLimit ) {
    kdDebug(7125) << "timelimit: " << mTimeLimit << endl;
    if ( ldap_set_option( mLDAP, LDAP_OPT_TIMELIMIT, &mTimeLimit ) != LDAP_SUCCESS ) {
      closeConnection();
      error( ERR_UNSUPPORTED_ACTION, 
        i18n("Cannot set time limit.").arg(version) );
      return;
    }
  }
  
  bool auth = false;
  bool firstauth = true;
  bool dlgResult;
  kldap_sasl_defaults defaults;
  QString mechanism = mMech.isEmpty() ? "DIGEST-MD5" : mMech;
  
  ret = LDAP_SUCCESS;
  while (!auth) {
    if ( mAuthSASL ) {
      kdDebug(7125) << "sasl_authentication mechanism:" << mechanism << endl;
#ifdef HAVE_SASL_H      
      defaults.realm = mRealm;
      defaults.authcid = mUser;
      defaults.passwd = mPassword;
      defaults.authzid = mBindName;
#else
      closeConnection();
      error( ERR_SLAVE_DEFINED, 
        i18n("SASL authentication not compiled into the ldap ioslave.") );
      return;
#endif      
    }
    kdDebug(7125) << "user: " << mUser << " bindname: " << mBindName << endl;
    if ( ( !mAuthSASL && mPassword.isEmpty() && !mBindName.isEmpty() ) || ( ret = ( 
#ifdef HAVE_SASL_H      
      mAuthSASL ? 
        ldap_sasl_interactive_bind_s( mLDAP, NULL, mechanism.utf8(), 
          NULL, NULL, LDAP_SASL_QUIET, &kldap_sasl_interact, &defaults ) :
#endif          
        ldap_simple_bind_s( mLDAP, mBindName.utf8(), mPassword.utf8() )
          == LDAP_INVALID_CREDENTIALS ) )) {
      
      kdDebug(7125) << "Auth error, open pass dlg! " << endl;
      if (firstauth)
        dlgResult = openPassDlg( info );
      else
        dlgResult = openPassDlg( info, i18n("Invalid authorization information.") );
      
      firstauth = false;
      if ( !dlgResult ) {
        // user canceled or dialog failed to open
        error( ERR_USER_CANCELED, QString::null );
        closeConnection();
        return;
      }
      if ( mAuthSASL )
        mUser = info.username;
      else
        mBindName = info.username;
      mPassword = info.password;
    } else {
      auth = true;
      if ( ret != LDAP_SUCCESS ) {
        LDAPErr( ret, Url.prettyURL() );
        closeConnection();
        return;
      }
    }
  }
  
  kdDebug(7125) << "connected!" << endl;
  connected();
}

void LDAPProtocol::closeConnection()
{
  if (mLDAP) ldap_unbind(mLDAP);
  mLDAP = 0;
  kdDebug(7125) << "connection closed!" << endl;
}

/**
 * Get the information contained in the URL.
 */
void LDAPProtocol::get( const KURL &_url )
{
  kdDebug(7125) << "get(" << _url << ")" << endl;

  LDAPUrl usrc(_url);
  int ret, id;
  LDAPMessage *msg,*entry;
  
  changeCheck( usrc );
  if ( !mLDAP ) { 
    finished();
    return;
  }
  
  if ( (id = asyncSearch( usrc )) == -1 ) {
    checkErr( _url );
    return;
  }

  // tell the mimetype
  mimeType("text/plain");
  // collect the result
  QCString result;
  KIO::filesize_t processed_size = 0;
  QByteArray array;
  
  while( true ) {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      checkErr( _url );
      return;
    }
    kdDebug(7125) << " ldap_result: " << ret << endl;
    if ( ret == LDAP_RES_SEARCH_RESULT ) break;
    if ( ret != LDAP_RES_SEARCH_ENTRY ) continue;
    
    entry = ldap_first_entry( mLDAP, msg );
    while ( entry ) {
      result = LDAPEntryAsLDIF(entry);
      result += '\n';
      uint len = result.length();
      processed_size += len;
      array.setRawData( result.data(), len );
      data(array);
      processedSize( processed_size );
      array.resetRawData( result.data(), len );
    
      entry = ldap_next_entry( mLDAP, entry );
    }
    checkErr( _url );
      
    ldap_msgfree(msg);
  // tell the length
  }
    
  totalSize(processed_size);

  array.resize(0);
  // tell we are finished
  data(array);
  
  // tell we are finished
  finished();
}

/**
 * Test if the url contains a directory or a file.
 */
void LDAPProtocol::stat( const KURL &_url )
{
  kdDebug(7125) << "stat(" << _url << ")" << endl;

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
    checkErr( _url );
    return;
  }
  
  kdDebug(7125) << "stat() getting result" << endl;
  do {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      checkErr( _url );
      return;
    }
    if ( ret == LDAP_RES_SEARCH_RESULT ) {
      ldap_msgfree( msg );
      error( ERR_DOES_NOT_EXIST, _url.prettyURL() );
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
void LDAPProtocol::del( const KURL &_url, bool )
{
  kdDebug(7125) << "del(" << _url << ")" << endl;

  LDAPUrl usrc(_url);
  int ret;

  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }
  
  kdDebug(7125) << " del: " << usrc.dn().utf8() << endl ;
  
  if ( (ret = ldap_delete_s( mLDAP,usrc.dn().utf8() )) != LDAP_SUCCESS ) {
    LDAPErr(ret,_url.prettyURL());
    return;
  }
  finished();
}

void LDAPProtocol::put( const KURL &_url, int, bool overwrite, bool )
{
  kdDebug(7125) << "put(" << _url << ")" << endl;

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
  LDIF::ParseVal ret;
  LDIF ldif;
  ret = LDIF::MoreData;
  int ldaperr;
  
  
  do {
    if ( ret == LDIF::MoreData ) {
      dataReq(); // Request for data
      result = readData( buffer );
    }
    if ( result < 0 ) {
//      error
      return;
    }
    if ( result == 0 ) {
      kdDebug(7125) << "EOF!" << endl;
      buffer.resize( 3 );
      buffer[ 0 ] = '\n';
      buffer[ 1 ] = '\n';
      buffer[ 2 ] = '\n';
    }
    ldif.setLDIF( buffer ); 
    do {
      
      ret = ldif.nextItem();
      kdDebug(7125) << "nextitem: " << ret << endl;
      
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
              return;
            case LDIF::Entry_Del:
              kdDebug(7125) << "kio_ldap_del" << endl;
              ldaperr = ldap_delete_ext_s( mLDAP, ldif.dn().utf8(), 
                serverctrls, clientctrls );
              ldap_controls_free( serverctrls );
              ldap_controls_free( clientctrls );
              serverctrls = 0; clientctrls = 0;
              break;
            case LDIF::Entry_Modrdn:
              kdDebug(7125) << "kio_ldap_modrdn olddn:" << ldif.dn() << 
                " newRdn: " <<  ldif.newRdn() << 
                " newSuperior: " << ldif.newSuperior() << 
                " deloldrdn: " << ldif.delOldRdn() << endl;
              ldaperr = ldap_rename_s( mLDAP, ldif.dn().utf8(), ldif.newRdn().utf8(), 
                ldif.newSuperior().isEmpty() ? 0 : ldif.newSuperior().utf8(), 
                ldif.delOldRdn(), serverctrls, clientctrls );

              ldap_controls_free( serverctrls );
              ldap_controls_free( clientctrls );
              serverctrls = 0; clientctrls = 0;
              break;
            case LDIF::Entry_Mod:
              kdDebug(7125) << "kio_ldap_mod"  << endl;
              if ( lmod ) {
                ldaperr = ldap_modify_ext_s( mLDAP, ldif.dn().utf8(), lmod,
                  serverctrls, clientctrls );
                ldap_mods_free( lmod, 1 );
                ldap_controls_free( serverctrls );
                ldap_controls_free( clientctrls );
                lmod = 0; serverctrls = 0; clientctrls = 0;
              }
              break;
            case LDIF::Entry_Add:
              kdDebug(7125) << "kio_ldap_add " << ldif.dn() << endl;
              if ( lmod ) {
                ldaperr = ldap_add_ext_s( mLDAP, ldif.dn().utf8(), lmod,
                  serverctrls, clientctrls );
                if ( ldaperr == LDAP_ALREADY_EXISTS && overwrite ) {
                  kdDebug(7125) << ldif.dn() << " already exists, delete first" << endl;
                  ldaperr = ldap_delete_s( mLDAP, ldif.dn().utf8() );
                  if ( ldaperr == LDAP_SUCCESS ) 
                    ldaperr = ldap_add_ext_s( mLDAP, ldif.dn().utf8(), lmod,
                      serverctrls, clientctrls );
                }
                ldap_mods_free( lmod, 1 );
                ldap_controls_free( serverctrls );
                ldap_controls_free( clientctrls );
                lmod = 0; serverctrls = 0; clientctrls = 0;
              }
              break;
          }
          if ( ldaperr != LDAP_SUCCESS ) {
            kdDebug(7125) << "put ldap error: " << ldap_err2string(ldaperr) << endl;
            LDAPErr( ldaperr, _url.prettyURL() );
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
              addModOp( &lmod, modtype, ldif.attr(), ldif.val() );
              break;
            }
            case LDIF::Entry_Add:
              if ( ldif.val().size() > 0 )
                addModOp( &lmod, 0, ldif.attr(), ldif.val() );
              break;
            default:
              error( ERR_INTERNAL, i18n("The LDIF parser failed.") );
              ldap_mods_free( lmod, 1 );
              ldap_controls_free( serverctrls );
              ldap_controls_free( clientctrls );
              lmod = 0; serverctrls = 0; clientctrls = 0;
              return;  
          }
          break;
        case LDIF::Control:
          addControlOp( &serverctrls, ldif.oid(), ldif.val(), ldif.critical() );
          break;
        case LDIF::Err:
          error( ERR_SLAVE_DEFINED, 
            i18n( "Invalid LDIF file in line %1." ).arg( ldif.lineNo() ) );
          ldap_mods_free( lmod, 1 );
          ldap_controls_free( serverctrls );
          ldap_controls_free( clientctrls );
          lmod = 0; serverctrls = 0; clientctrls = 0;
          return;
      }
    } while ( ret != LDIF::MoreData );
  } while ( result > 0 );
              
  finished();
}

/**
 * List the contents of a directory.
 */
void LDAPProtocol::listDir( const KURL &_url )
{
  int ret, ret2, id, id2;
  unsigned long total=0;
  char *dn;
  QStringList att,saveatt;
  LDAPMessage *entry,*msg,*entry2,*msg2;
  LDAPUrl usrc(_url),usrc2(_url);
  bool critical;
  bool isSub = ( usrc.extension( "x-dir", critical ) == "sub" );
  
  kdDebug(7125) << "listDir(" << _url << ")" << endl;
  
  changeCheck( usrc );
  if ( !mLDAP ) {
    finished();
    return;
  }

  saveatt = usrc.attributes();
  // look up the entries
  if ( isSub ) {
    att.append("dn");
    usrc.setAttributes(att);  
  }
  if ( _url.query().isEmpty() ) usrc.setScope( LDAPUrl::One );
  
  if ( (id = asyncSearch( usrc )) == -1 ) {
    checkErr( _url );
    return;
  }

  usrc.setAttributes( "" );
  usrc.setExtension( "x-dir", "base" );
  // publish the results
  UDSEntry uds;

  while( true ) {
    ret = ldap_result( mLDAP, id, 0, NULL, &msg );
    if ( ret == -1 ) {
      checkErr( _url );
      return;
    }
    if ( ret == LDAP_RES_SEARCH_RESULT ) break;
    if ( ret != LDAP_RES_SEARCH_ENTRY ) continue;
    kdDebug(7125) << " ldap_result: " << ret << endl;
    
    entry = ldap_first_entry( mLDAP, msg );
    while( entry ) {
  
      total++;
      uds.clear();
    
      dn = ldap_get_dn( mLDAP, entry );
      kdDebug(7125) << "dn: " << dn  << endl;
      LDAPEntry2UDSEntry( QString::fromUtf8(dn), uds, usrc );
      listEntry( uds, false );
//      processedSize( total );
      kdDebug(7125) << " total: " << total << " " << usrc.prettyURL() << endl;
    
    // publish the sub-directories (if dirmode==sub)
      if ( isSub ) {
        usrc2.setDn( QString::fromUtf8( dn ) );
        usrc2.setScope( LDAPUrl::One );
        usrc2.setAttributes( att );
        usrc2.setFilter( QString::null );
        kdDebug(7125) << "search2 " << dn << endl;
        if ( (id2 = asyncSearch( usrc2 )) != -1 ) {
          while ( true ) {
            kdDebug(7125) << " next result " << endl;
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
    checkErr( _url );
    ldap_msgfree( msg );
  }
  
//  totalSize( total );
  
  uds.clear();
  listEntry( uds, true );
  // we are done
  finished();
}
