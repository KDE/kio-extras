// $Id$

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

#include <lber.h>
#include <ldap.h>
#include <kdebug.h>
#include "kldap.h"
#include "kldapurl.h"
#include <kinstance.h>

#include "kio_ldap.h"

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

/**
 * The main program.
 */
int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_ldap" );

  kdDebug(7125) << "kio_ldap : Starting " << getpid() << endl;

  if ( argc != 4 ) {
    kdError() << "Usage kio_ldap protocol pool app" << endl;
    return -1;
  }

  // let the protocol class do its work
  LDAPProtocol slave( argv[ 2 ], argv[ 3 ] );
  slave.dispatchLoop();

  kdDebug( 7125 ) << "kio_ldap : Done" << endl;
  return 0;
}


/**
 * Initialize the ldap slave
 */
LDAPProtocol::LDAPProtocol(const QCString &pool, const QCString &app)
  : SlaveBase( "ldap", pool, app)
{
  kdDebug(7125) << "LDAPProtocol::LDAPProtocol" << endl;
}

void LDAPProtocol::setHost( const QString& host, int port,
                            const QString& user, const QString& password )
{
  mUser = user;
  mPassword = password;

  mUrlPrefix = "ldap://";
  if (!user.isEmpty()) {
    mUrlPrefix += user;
    if (!password.isEmpty())
      mUrlPrefix += ":" + password;
    mUrlPrefix += "@";
  }
  mUrlPrefix += host;
  if (port)
    mUrlPrefix += QString( ":%1" ).arg( port );
  kdDebug(7125) << "mUrlPrefix " << mUrlPrefix << endl;
}

/**
 * Get the information contained in the URL.
 */
void LDAPProtocol::get(const KURL &_url)
{
  kdDebug(7125) << "kio_ldap::get(" << _url << ")" << endl;
  KLDAP::Url usrc(_url);
  usrc.setBindDN( mUser );
  usrc.setPwdBindDN( mPassword );

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, _url.prettyURL());
    return;
  }

  KLDAP::Connection c;

  KLDAP::SearchRequest search(c, usrc, KLDAP::Request::Synchronous);

  // Check for connection errors
  if( c.handle() == 0 ) {
    switch( errno ) {
    case ECONNREFUSED:
      error( ERR_COULD_NOT_CONNECT, _url.prettyURL() );
      return;
    default:
      error( ERR_UNKNOWN_HOST, _url.prettyURL() );
      return;
    }
  }

  // wait for the request
  if( !search.execute() ) {
    int res = search.result();
    switch( res ) {
    case LDAP_OPERATIONS_ERROR:
    case LDAP_PROTOCOL_ERROR:
      error( ERR_INTERNAL, _url.prettyURL() );
      return;
    case LDAP_SERVER_DOWN:
      error( ERR_COULD_NOT_BIND, _url.prettyURL() );
      return;
    case LDAP_INVALID_SYNTAX:
    case LDAP_INVALID_DN_SYNTAX:
      error( ERR_MALFORMED_URL, _url.prettyURL() );
      return;
    case LDAP_TIMELIMIT_EXCEEDED:
    case LDAP_SIZELIMIT_EXCEEDED:
      /* ... */
      /* we try to ignore those */
      break;
    }
  }
  if( !search.finish() ) {
    // Could not finish query
    error( ERR_COULD_NOT_READ, _url.prettyURL() );
    return;
  }

  // collect the result
  QCString result = search.asLDIF();

  // tell the mimetype
  mimeType("text/plain");

  // tell the length
  int processed_size = result.length();
  totalSize(processed_size);

  // tell the contents of the URL
  QByteArray array;
  array.setRawData( result.data(), result.length() );
  data(array);
  array.resetRawData( result.data(), result.length() );
  processedSize( processed_size );
  // tell we are finished
  data(QByteArray());

  // tell we are finished
  finished();
}


/**
 * Test if the url contains a directory or a file.
 */
void LDAPProtocol::stat( const KURL &_url )
{
  /*QString _url = mUrlPrefix + path;
  if (!query.isEmpty()) { _url += "?" + query; }*/
  kdDebug(7125) << "kio_ldap: stat(" << _url << ")" << endl;
  KLDAP::Url usrc(_url);
  usrc.setBindDN( mUser );
  usrc.setPwdBindDN( mPassword );

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, _url.prettyURL());
    return;
  }

  // look how many entries match
  KLDAP::Connection c;

  KLDAP::SearchRequest search(c, usrc, KLDAP::Request::Synchronous);
  QStringList att;
  att.append("dn");
  search.setAttributes(att);
  if (_url.query().isEmpty()) search.setScope(LDAP_SCOPE_ONELEVEL);
  search.execute();
  search.finish();
  int cnt=0;
  for (KLDAP::Entry e=search.first(); !search.end(); e=search.next())
    cnt++;
  int isDir = 1;
  bool isQuery = 0;
  if (_url.query().isEmpty()) {
    /* we searched for a subdir */
    if (cnt == 0) isDir=0;
  } else {
    /* we searched for what the user specified */
    if (usrc.scope() == LDAP_SCOPE_BASE) isDir = 0;    /* he only wanted base */
    else {
      /* he wanted more */
      if (cnt <= 1) isDir = 0;   /* but there isn't */
      else isQuery = 1;          /* e.g. /cn=bla??sub?(uid=23) */
    }
  }
  UDSEntry entry;
  UDSAtom atom;

  int pos;
  atom.m_uds = UDS_NAME;
  atom.m_long = 0;
  QString name = usrc.dn();
  if ((pos = name.find(",")) > 0)
    name = name.left(pos);
  if ((pos = name.find("=")) > 0)
    name.remove(0,pos+1);
  atom.m_str = name.replace(' ', "_") + ".ldif";
  entry.append(atom);

  atom.m_uds = UDS_FILE_TYPE;
  atom.m_str = "";
  if (isQuery) {
    if (cnt > 1)
      atom.m_long = S_IFDIR;
    else
      atom.m_long = S_IFREG;
  } else if (isDir)
    atom.m_long = S_IFDIR;
  else
    atom.m_long = S_IFREG;
  entry.append(atom);

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = isDir ? 0500 : 0400;
  entry.append(atom);

  atom.m_uds = UDS_URL;
  atom.m_long = 0;
  KLDAP::Url url(mUrlPrefix);
  url.setHost(usrc.host());
  url.setPort(usrc.port());
  url.setPath("/"+usrc.dn());
  if (isQuery)
    url.setScope(usrc.scope());
  else if (isDir)
    url.setScope(LDAP_SCOPE_ONELEVEL);
  else
    url.setScope(LDAP_SCOPE_BASE);
  atom.m_str = url.prettyURL();
  kdDebug(7125) << "kio_ldap:stat put url:" << atom.m_str << endl;
  entry.append(atom);

  if (!isDir || (isQuery && cnt==1)) {
    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/plain";
    entry.append(atom);
  }

  statEntry(entry);
  // we are done
  finished();
}

#if 0
/**
 * Get the mimetype. For now its text/plain for each non-subentry
 */
void LDAPProtocol::mimetype(const KURL &url)
{
  /*QString _url = mUrlPrefix + path;
  if (!query.isEmpty()) { _url += "?" + query; }*/
  QString _url = url.prettyURL();
  kdDebug(7125) << "kio_ldap: mimetype(" << _url << ")" << endl;
  KLDAP::Url usrc(_url);
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, _url);
    return;
  }
  kdDebug(7125) << "kio_ldap: query()==" << url.query() << endl;
  if (!url.query().isEmpty()) {
    if (usrc.scope() == LDAP_SCOPE_BASE)
      mimeType("text/plain");
    else if (usrc.scope() == LDAP_SCOPE_SUBTREE)
      mimeType("text/plain");
  } else {
    /* empty scope, or ONELEVEL/SUB */
    mimeType("inode/directory");
  }
  finished();
}
#endif

/**
 * List the contents of a directory.
 */
void LDAPProtocol::listDir(const KURL &_url)
{
  unsigned long total=0, actual=0, dirs=0;
  kdDebug(7125) << "kio_ldap: listDir(" << _url << ")" << endl;
  KLDAP::Url usrc(_url);
  usrc.setBindDN( mUser );
  usrc.setPwdBindDN( mPassword );

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, _url.prettyURL());
    return;
  }

  // look up the entries
  KLDAP::Connection c;

  KLDAP::SearchRequest search(c, usrc, KLDAP::Request::Synchronous);
  QStringList att;
  att.append("dn");
  search.setAttributes(att);
  if (_url.query().isEmpty() || usrc.scope() == LDAP_SCOPE_BASE)
    search.setScope(LDAP_SCOPE_ONELEVEL);
  search.execute();
  search.finish();

  // publish the results
  UDSEntry entry;
  UDSAtom atom;

  // publish the sub-directories
  for (KLDAP::Entry e=search.first(); !search.end(); e=search.next())
    {
      total++;
      totalSize(total+dirs);
      entry.clear();

      // test if it is really a directory (NOTE: This is expensive!)
      KLDAP::SearchRequest search2(c, usrc, KLDAP::Request::Synchronous);
      search2.setBase(e.dn());
      search2.setScope(LDAP_SCOPE_ONELEVEL);
      search2.setAttributes(att);
      search2.execute();
      search2.finish();
      int cnt=0;
      for (KLDAP::Entry e2=search2.first(); !search2.end(); e2=search2.next())
	cnt++;

      if (cnt > 0)
	{
	  dirs++;
	  totalSize(total+dirs);
	  // the name
	  int pos;
	  atom.m_uds = UDS_NAME;
	  atom.m_long = 0;
	  QString name = e.dn();
	  if ((pos = name.find(",")) > 0)
	    name = name.left(pos);
	  if ((pos = name.find("=")) > 0)
	    name.remove(0,pos+1);
	  atom.m_str = name;
	  entry.append(atom);

	  // the file type
	  atom.m_uds = UDS_FILE_TYPE;
	  atom.m_str = "";
	  atom.m_long = S_IFDIR;
	  entry.append(atom);

          atom.m_uds = KIO::UDS_ACCESS;
          atom.m_long = 0500;
          entry.append(atom);

	  // the url
	  atom.m_uds = UDS_URL;
	  atom.m_long = 0;
	  KLDAP::Url url(mUrlPrefix);
	  url.setHost(usrc.host());
	  url.setPort(usrc.port());
	  url.setPath("/"+e.dn());
	  url.setScope(LDAP_SCOPE_SUBTREE);
	  atom.m_str = url.prettyURL();
	  kdDebug(7125) << "kio_ldap:listDir(dir) put url:" << atom.m_str << endl;
	  entry.append(atom);

	  listEntry(entry, false);
	  actual++;
	  processedSize(actual);
	}
    }

  totalSize(total+dirs);
  actual = dirs;
  processedSize(actual);
  // publish the nodes
  for (KLDAP::Entry e=search.first(); !search.end(); e=search.next())
    {
      entry.clear();

      // the name
      int pos;
      atom.m_uds = UDS_NAME;
      atom.m_long = 0;
      QString name = e.dn();
      if ((pos = name.find(",")) > 0)
	name = name.left(pos);
      if ((pos = name.find("=")) > 0)
	name.remove(0,pos+1);
      atom.m_str = name.replace(' ', "_") + ".ldif";
      entry.append(atom);

      // the file type
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_str = "";
      atom.m_long = S_IFREG;
      entry.append(atom);

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = 0400;
      entry.append(atom);

      // the mimetype
      atom.m_uds = UDS_MIME_TYPE;
      atom.m_long = 0;
      atom.m_str = "text/plain";
      entry.append(atom);

      // the url
      atom.m_uds = UDS_URL;
      atom.m_long = 0;
      KLDAP::Url url(mUrlPrefix);
      //url.setProtocol("ldap");
      url.setHost(usrc.host());
      url.setPort(usrc.port());
      url.setPath("/"+e.dn());
      url.setScope(LDAP_SCOPE_BASE);
      atom.m_str = url.prettyURL();
      kdDebug(7125) << "kio_ldap:listDir(file) put url:" << atom.m_str << endl;
      entry.append(atom);

      listEntry(entry, false);
      actual++;
      processedSize(actual);
    }
  entry.clear();
  listEntry(entry, true);
  processedSize(total+dirs);
  // we are done
  finished();
}
