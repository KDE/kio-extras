// $Id$

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <signal.h>

#include <lber.h>
#include <ldap.h>
#include <kldap.h>
#include <kldapurl.h>

#include "kio_ldap.h"


/**
 * The main program.
 */
int main(int, char **)
{
  // redirect the signals
  signal(SIGCHLD, KIOProtocol::sigchld_handler);
  signal(SIGSEGV, KIOProtocol::sigsegv_handler);

  qDebug("kio_ldap : Starting");

  // create a connection between slave on parent
  KIOConnection parent(0, 1);
  
  // let the protocol class do its work
  LDAPProtocol file(&parent);
  file.dispatchLoop();

  qDebug("kio_ldap : Done");
}


/**
 * Initialize the protocol with a Connection.
 */
LDAPProtocol::LDAPProtocol(KIOConnection *_conn) 
  : KIOProtocol(_conn )
{
  m_bIgnoreJobErrors = FALSE;
}


/**
 * Get the information contained in the URL.
 */
void LDAPProtocol::slotGet(const char *_url)
{
  qDebug("kio_ldap: slotGet(%s)\n", _url);
  
  KLDAP::Url usrc(_url);

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, strdup(_url));
    return;
  }

  // take the time
  time_t t_start = time( 0L );

  // initiate the search
  KLDAP::Connection c;
  KLDAP::SearchRequest search(c, _url, KLDAP::Request::Synchronous);

  // tell that we are ready and getting the data
  ready();
  gettingFile(_url);

  // wait for the request
  search.execute();
  search.finish();

  // collect the result
  QString result = search.asLDIF();

  // tell the mimetype
  //  mimeType("text/ldif");

  // tell the length
  int processed_size = result.length();
  totalSize(processed_size);  

  // tell the contents of the URL
  int cnt=0;
  while (cnt < processed_size)
    {
      if (result.length()-cnt > 1024)
	{
	  data(result.mid(cnt,1024).ascii(), 1024);
	  cnt += 1024;
	}
      else
	{
	  data(result.ascii(), result.length()-cnt);
	  cnt = processed_size;
	}
      // tell how much we got
      processedSize(cnt);
    }

  // tell we are finished
  dataEnd();
  
  // tell how long it took
  time_t t = time( 0L );
  if ( t - t_start >= 1 )
    speed( processed_size / ( t - t_start ) );  

  // tell we are finished
  finished();
}


/**
 * Test if the url contains a directory or a file.
 */
void LDAPProtocol::slotTestDir( const char *_url )
{
  qDebug("kio_ldap: slotTestDir(%s)\n", _url);

  KLDAP::Url usrc(_url);

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, strdup(_url));
    return;
  }

  // look how many entries match
  KLDAP::Connection c;
  KLDAP::SearchRequest search(c, _url, KLDAP::Request::Synchronous);
  QStrList att;
  att.append("dn");
  search.setAttributes(att);
  search.execute();
  search.finish();
  int cnt=0;
  for (KLDAP::Entry e=search.first(); !search.end(); e=search.next())
    cnt++;
  
  if (cnt == 1)
    isFile();
  else 
    isDirectory();

  // we are done
  finished();
}


/**
 * List the contents of a directory.
 */
void LDAPProtocol::slotListDir(const char *_url)
{
  qDebug("kio_ldap: slotListDir(%s)\n", _url);

  KLDAP::Url usrc(_url);

  // check if the URL is a valid LDAP URL
  if (usrc.isMalformed()) {
    error(ERR_MALFORMED_URL, strdup(_url));
    return;
  }

  // look up the entries
  KLDAP::Connection c;
  KLDAP::SearchRequest search(c, _url, KLDAP::Request::Synchronous);
  QStrList att;
  att.append("dn");
  search.setAttributes(att);
  search.execute();
  search.finish();

  // publish the results
  KUDSEntry entry;
  KUDSAtom atom;

  // publish the directories
  for (KLDAP::Entry e=search.first(); !search.end(); e=search.next())
    {
      entry.clear();

      // test if it is really a directory (NOTE: This is expensive!)
      KLDAP::SearchRequest search2(c, usrc.url(), KLDAP::Request::Synchronous);
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
	  
	  // the url
	  atom.m_uds = UDS_URL;
	  atom.m_long = 0;
	  KLDAP::Url url("");
	  url.setProtocol("ldap");
	  url.setHost(usrc.host());
	  url.setPort(usrc.port());
	  url.setPath(e.dn());
	  url.setScope(LDAP_SCOPE_ONELEVEL);
	  atom.m_str = url.url();
	  entry.append(atom);

	  listEntry(entry);
	}
    }

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
      atom.m_str = name;
      entry.append(atom);

      // the file type
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_str = "";
      atom.m_long = S_IFREG;
      entry.append(atom);

      // the mimetype
      atom.m_uds = UDS_MIME_TYPE;
      atom.m_long = 0;
      atom.m_str = "text/ldif";
      entry.append(atom);

      // the url
      atom.m_uds = UDS_URL;
      atom.m_long = 0;
      KLDAP::Url url("");
      url.setProtocol("ldap");
      url.setHost(usrc.host());
      url.setPort(usrc.port());
      url.setPath(e.dn());
      url.setScope(LDAP_SCOPE_BASE);
      atom.m_str = url.url();
      entry.append(atom);

      listEntry(entry);
    }
  
  // we are done
  finished();  
}


/**
 * Report errors.
 */
void LDAPProtocol::jobError(int _errid, const char *_txt)
{
  if ( !m_bIgnoreJobErrors )
    error( _errid, _txt );
}


/*************************************
 *
 * FileIOJob
 *
 *************************************/

LDAPIOJob::LDAPIOJob( KIOConnection *_conn, LDAPProtocol *_File ) : KIOJobBase( _conn )
{
  m_pFile = _File;
}
  
void LDAPIOJob::slotError( int _errid, const char *_txt )
{
  KIOJobBase::slotError( _errid, _txt );
  m_pFile->jobError( _errid, _txt );
}

