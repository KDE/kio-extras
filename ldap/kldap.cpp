#include <config.h>
#include <stdlib.h>
#include <qtextstream.h>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#include <time.h>
#include "kldap.h"
#include "kldapurl.h"
#include <kdebug.h>
#include <kmdcodec.h>

using namespace KLDAP;

LDAPBase::LDAPBase()
  : res(0), _handle(0)
{
}


bool LDAPBase::check(int r)
{
  // store error code for later retrieval
  res = r;

  // output result
  kdDebug() << ": " << error() << endl;

  // succeeded?
  return r == LDAP_SUCCESS;
}


QString LDAPBase::error()
{
  return ldap_err2string(res);
}


KLDAP::Connection::Connection(const char *s, int p)
  : LDAPBase(), _server(s), _port(p)
{
}


KLDAP::Connection::~Connection()
{
  disconnect();
}


bool KLDAP::Connection::connect()
{
  // if we are already connected, it is better to disconnect now
  if (handle())
    disconnect();

  // try to connect to the server
  _handle = ldap_open(const_cast<char*>(_server.ascii()), _port);

  kdDebug() << "open connection to " << _server << ":" << _port;
  kdDebug() << ((handle() != 0) ? " succeeded" : " failed") << endl;

  // test if connect succeeded
  return handle() != 0;
}


bool KLDAP::Connection::disconnect()
{
  // test if we are really connected
  if (!handle())
    return TRUE;

  kdDebug() << "close connection to " << _server << ":" << _port;

  // close the connection to the server
  check(ldap_unbind(handle()));
  _handle = 0;

  return result() == LDAP_SUCCESS;
}


bool KLDAP::Connection::authenticate(const char *dn, const char *cred, int method)
{
  if (!handle())
    return FALSE;

  kdDebug() << "authentication";

  return check(ldap_bind_s(handle(), const_cast<char*>(dn), const_cast<char*>(cred), method));
}


Request::Request(Connection &c, RunMode m)
  : LDAPBase(), mode(m), running(FALSE), id(0), all(1), 
    req_result(0), use_timeout(FALSE)
{
  to_sec = 0;
  to_usec = 0;
  
  _handle = c.handle();
  expected = -2;
}


Request::~Request()
{
  if (req_result)
    ldap_msgfree(req_result);
}


bool Request::execute()
{
  // if there was already a request: stop it
  if (running)
    abandon();

  // mark the request as running
  running = TRUE;

  return TRUE;
}


bool Request::finish()
{
  if (!handle())
    return FALSE;

  kdDebug() << "finish request" << endl;

  // if sync, the result is already there
  // if not: get the result
  if (mode == Asynchronous)
    {
      int retval;

      // was there really a request?
      if (!id)
	return FALSE;

      // delete previous result
      if (req_result)
	ldap_msgfree(req_result);

      // get the result
      if (useTimeout()) {
        struct timeval to;
	to.tv_sec = to_sec;
	to.tv_usec = to_usec;
	retval = ldap_result(handle(), id, all, &to, &req_result);
      } else
	retval = ldap_result(handle(), id, all, 0, &req_result);
      
      // check if there was an error
      if (retval == -1)
	{
	  running = FALSE;
	  id = 0;
	  return check(ldap_result2error(handle(), req_result, 1));
	}

      // check if the timeout was exceeded
      if (retval == 0)
	return FALSE;
	
      // check if the result was the one expected
      if (retval != expected)
	return FALSE;
    }

  // the result should be ready now
  if (!req_result)
    return FALSE;

  return TRUE;
}


bool Request::abandon()
{
  if (!handle())
    return FALSE;

  // check only if async
  if (mode == Asynchronous)
    {
      // was there really a request?
      if (!id)
	return FALSE;

      // cancel the request
      id = 0;
      running = FALSE;
      return ldap_abandon(handle(), id);
    }

  return TRUE;
}


SearchRequest::SearchRequest(Connection &c, RunMode m)
  : Request(c,m), _base(""), _filter("(objectClass=*)"), _scope(LDAP_SCOPE_SUBTREE),
    _attrsonly(0), entry(0)
{
  expected = LDAP_RES_SEARCH_RESULT;

  // try to connect
  if (!c.isConnected())
    c.connect();
  _handle = c.handle();
}


SearchRequest::SearchRequest(Connection &c, QString _url, RunMode m)
  : Request(c,m), _base(""), _filter("(objectClass=*"), _scope(LDAP_SCOPE_SUBTREE),
    _attrsonly(0), entry(0)
{
  Url url(_url);

  expected = LDAP_RES_SEARCH_RESULT;

  // close the connection if not to the right host/port
  if (url.host() != c.host() || url.port() != c.port())
    {
      if (c.isConnected())
	c.disconnect();
      c.setHost(url.host());
      c.setPort(url.port());
    }

  // try to connect
  if (!c.isConnected())
    c.connect();
  _handle = c.handle();
  
  // TODO: authenticate via the basename extension!!!!!

  // set the search criteria
  setBase(url.dn());
  setScope(url.scope());
  setFilter(url.filter());
  setAttributes(url.attributes());
}


bool SearchRequest::execute()
{
  if (!handle())
    return FALSE;

  // call the inherited method
  Request::execute();

  kdDebug() << "search request: base=\"" << _base << "\" scope=\"" << _scope 
	    << "\" filter=\"" << _filter << "\"" << endl;

  // Honour the attributes to return
  char **attrs = 0;
  int count = _attributes.count();
  if (count > 0)
    {
      attrs = static_cast<char**>(malloc((count+1) * sizeof(char*)));
      for (int i=0; i<count; i++)
	  attrs[i] = strdup(_attributes.at(i));
      attrs[count] = 0;
    }  
  
  // if async, just issue the request
  if (mode == Asynchronous)
    {
      // start searching
      id = ldap_search(handle(), const_cast<char*>(_base.ascii()), _scope, 
		       const_cast<char*>(_filter.ascii()), attrs, _attrsonly);

      // free the attributes list again
      if (count > 0)
	{
	  for (int i=0; i<count; i++)
	    free(attrs[i]);
	  free(attrs);
	}
      
      // check for an error
      if (id == -1)
	{
	  id = 0;
	  running = FALSE;
	  return FALSE; /* ldap_search returning -1 is ALWAYS trouble */
	}
      return TRUE;
    }

  int retval;

  // call the right version
  if (useTimeout())
    {
      struct timeval to;
      to.tv_sec = to_sec;
      to.tv_usec = to_usec;
      retval = ldap_search_st(handle(), const_cast<char*>(_base.ascii()), _scope, 
			      const_cast<char*>(_filter.ascii()), attrs, 
			      _attrsonly, &to, &req_result);
    }
  else
    {
      retval = ldap_search_s(handle(), const_cast<char*>(_base.ascii()), _scope, 
			     const_cast<char*>(_filter.ascii()), attrs,
			     _attrsonly, &req_result);
    }

  // free the attributes list again
  if (count > 0)
    {
      for (int i=0; i<count; i++)
	free(attrs[i]);
      free(attrs);
    }
  
  running = FALSE;

  return check(retval);
}


bool SearchRequest::search(QString base, QString filter)
{
  kdDebug() << "search: base=" << base << " filter=" << filter;

  setBase(base);
  setFilter(filter);

  bool retval = execute();

  kdDebug() << (retval ? ": Success" : "Failed") << endl;
  
  return retval;
}


Attribute::Attribute(LDAP *h, LDAPMessage *msg, const char *n)
  : LDAPBase()
{
  _handle = h;
  message = msg;
  _name = const_cast<char*>(n);
}


void Attribute::getValues(QStrList &list)
{
  char **vals;
  
  list.clear();
  
  vals = ldap_get_values(handle(), message, _name);
  if (vals) 
    for (int i=0; vals[i] != 0; i++)
      list.append(vals[i]);
  ldap_value_free(vals);
}


struct berval **Attribute::getBinaryValues()
{
  return ldap_get_values_len(handle(), message, _name);
}


void Attribute::freeBinaryValues(struct berval **vals)
{
  ldap_value_free_len(vals);
}


Entry::Entry(LDAP *h, LDAPMessage *msg)
  : LDAPBase(), message(msg)
{
  _handle = h;
}

QString Entry::dn()
{
  return ldap_get_dn(handle(), message);
}


void Entry::getAttributes(QStrList &list)
{
  BerElement *entry;
  char       *name;

  list.clear();

  name = ldap_first_attribute(handle(), message, &entry);
  while (name != 0)
    {
      list.append(name);
      name = ldap_next_attribute(handle(), message, entry);
    }
}


Attribute Entry::getAttribute(const char *name)
{
  return Attribute(handle(), message, name);
}


Entry SearchRequest::first()
{
  entry = ldap_first_entry(handle(), req_result);
  return Entry(handle(), entry);
}


Entry SearchRequest::next()
{
  entry = ldap_next_entry(handle(), entry);
  return Entry(handle(), entry);
}


bool SearchRequest::end()
{
  return entry == 0;
}


static QCString breakIntoLines( const QCString& str )
{
  QCString result;
  int i;
  for( i = 0; i < str.length()-72; i += 72 ) {
    result += str.mid( i, 72 );
    result += "\n ";
  }
  result += str.mid( i );
  result += '\n';
  return result;
}

QCString SearchRequest::asLDIF()
{
  QCString         result;
  BerElement     *entry;
  char           *name;
  struct berval **bvals;

  LDAPMessage *item = ldap_first_entry(handle(), req_result);
  while (item)
    {
      // print the dn
      char* dn = ldap_get_dn(handle(), item);
      result += "dn: "; result += dn; result += '\n';
      //kdDebug() << "Outputting dn: \"" << dn << "\"" << endl;
      ldap_memfree( dn );

      // iterate over the attributes    
      name = ldap_first_attribute(handle(), item, &entry);
      while (name != 0)
	{
	  // print the values
	  bvals = ldap_get_values_len(handle(), item, name);
	  if (bvals) {
	    for (int i=0; bvals[i] != 0; ++i) {
	      char* val = bvals[i]->bv_val;
	      unsigned long len = bvals[i]->bv_len;
	      bool printable = true;
	      for( int j = 0; j < len; ++j ) {
		if(!val[j]||!QChar( val[j] ).isPrint()) {
		  printable = false;
		  break;
		}
	      }
	      if( printable ) {
		// There should be no NULLs in here
		QByteArray tmp;
		tmp.setRawData( val, len );
		//os << name << ": " << QCString(tmp) << endl;
		result += name; result += ": "; result += tmp; result += '\n';
		tmp.resetRawData( val, len );
	      } else {		
		QByteArray tmp;
		tmp.setRawData( val, len );
		QCString tmp2 = breakIntoLines( KCodecs::base64Encode( tmp, false ));
		tmp.resetRawData( val, len );
		//os << name << ":: " << endl;
		//os << " " << tmp2 << endl;
		result += name;	result += ":: \n "; result += tmp2; result += '\n';
	      }
	    }
	    ldap_value_free_len(bvals);
	  }
	  // next attribute
	  name = ldap_next_attribute(handle(), item, entry);
	}

      // next entry
      result += '\n';
      item = ldap_next_entry(handle(), item);
    }
  //kdDebug() << "result=\"" << result << "\"" << endl;  
  return result;
}
