#include <stdlib.h>
#include <qtextstream.h>

#include "kldap.h"
#include "kldapurl.h"
#include "debug.h"


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
  debug << ": " << error() << endl;

  // succeeded?
  return r == LDAP_SUCCESS;
}


QString LDAPBase::error()
{
  return ldap_err2string(res);
}


Connection::Connection(const char *s, int p)
  : LDAPBase(), _server(s), _port(p)
{
}


Connection::~Connection()
{
  disconnect();
}


bool Connection::connect()
{
  // if we are already connected, it is better to disconnect now
  if (handle())
    disconnect();

  // try to connect to the server
  _handle = ldap_open(const_cast<char*>(_server.ascii()), _port);

  debug << "open connection to " << _server << ":" << _port;
  debug << ((handle() != 0) ? " succeeded" : " failed") << endl;

  // test if connect succeeded
  return handle() != 0;
}


bool Connection::disconnect()
{
  // test if we are really connected
  if (!handle())
    return TRUE;

  debug << "close connection to " << _server << ":" << _port;

  // close the connection to the server
  check(ldap_unbind(handle()));
  _handle = 0;

  return result() == LDAP_SUCCESS;
}


bool Connection::authenticate(const char *dn, const char *cred, int method)
{
  if (!handle())
    return FALSE;

  debug << "authentication";

  return check(ldap_bind_s(handle(), const_cast<char*>(dn), const_cast<char*>(cred), method));
}


Request::Request(Connection &c, RunMode m)
  : LDAPBase(), mode(m), running(FALSE), id(0), all(1), 
    req_result(0), use_timeout(FALSE)
{
  _timeout.tv_sec = 0;
  _timeout.tv_usec = 0;
  
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

  debug << "finish request" << endl;

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
      if (use_timeout)
	retval = ldap_result(handle(), id, all, &_timeout, &req_result);
      else
	retval = ldap_result(handle(), id, all, 0, &req_result);
      
      // check if there was an error
      if (retval == -1)
	{
	  running = FALSE;
	  id = 0;
	  return check(handle()->ld_errno);
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


SearchRequest::SearchRequest(Connection &c, const char *_url, RunMode m)
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

  debug << "search request: " << _base << " " << _scope << " " << _filter << endl;

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
	  return check(handle()->ld_errno);
	}
      return TRUE;
    }

  int retval;

  // call the right version
  if (useTimeout())
    {
      retval = ldap_search_st(handle(), const_cast<char*>(_base.ascii()), _scope, 
			      const_cast<char*>(_filter.ascii()), attrs, 
			      _attrsonly, &_timeout, &req_result);
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
  debug << "search: base=" << base << " filter=" << filter;

  setBase(base);
  setFilter(filter);

  bool retval = execute();

  debug << (retval ? ": Success" : "Failed") << endl;
  
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


QString SearchRequest::asLDIF()
{
  QString result;
  QTextOStream os(&result);
  BerElement *entry;
  char       *name;
  char       **vals;

  LDAPMessage *item = ldap_first_entry(handle(), req_result);
  while (item)
    {
      // print the dn
      os << "dn: " <<  ldap_get_dn(handle(), item) << endl;

      // iterate over the attributes    
      name = ldap_first_attribute(handle(), item, &entry);
      while (name != 0)
	{
	  // print the values
	  vals = ldap_get_values(handle(), item, name);
	  if (vals) 
	    for (int i=0; vals[i] != 0; i++)
		os << name << ": " << vals[i] << endl;
	  ldap_value_free(vals);
	  
	  // next attribute
	  name = ldap_next_attribute(handle(), item, entry);
	}

      // next entry
      os << endl;
      item = ldap_next_entry(handle(), item);
    }

  return result;
}
