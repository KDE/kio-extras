#include <stream.h>

#include <lber.h>
#include <ldap.h>

#include "kldapurl.h"


using namespace KLDAP;


Url::Url(QString _url)
  : KURL(_url), _dn(""), _filter("objectClass=*"), _extensions("")
{
  parseLDAP();
}


void Url::splitString(QString q, char c, QStrList &list)
{
  int pos;
  QString item;

  while ( (pos = q.find(c)) >= 0)
    {
      item = q.left(pos);
      list.append(item);
      q.remove(0,pos+1);
    }
  list.append(q);
}


void Url::parseLDAP()
{
  // extract the dn
  _dn = path();
  if (_dn.left(1) == "/")
    _dn.remove(0,1);  

  // parse the query  
  QString q = query();

  // split into a list
  QStrList url_items;
  splitString(q, '?', url_items);
  
  // first come the attributes to query
  _attributes.clear();
  if (url_items.count() >= 1)
    {
      q = url_items.at(0);
      if (q.left(1) == "(")
	q.remove(0,1);
      if (q.right(1) == ")")
	q.remove(q.length()-1,1);
      if (!q.isEmpty())
	splitString(q, ',', _attributes);
    }

  // second the scope
  _scope = LDAP_SCOPE_BASE;
  if (url_items.count() >= 2)
    {
      if (!strcmp(url_items.at(1),"sub"))
	_scope = LDAP_SCOPE_SUBTREE;
      if (!strcmp(url_items.at(1),"one"))
	_scope = LDAP_SCOPE_ONELEVEL;
    }

  // third is simply the filter
  _filter = "(objectClass=*)";
  if (url_items.count() >= 3)
    _filter = url_items.at(2);
  if (_filter.isEmpty())
    _filter = "(objectClass=*)";
}


QStrList &Url::attributes()
{
  _attr_decoded.clear();
  
  for (char *it=_attributes.first(); it; it=_attributes.next())
    {
      QString item(it);
      decode(item);
      _attr_decoded.append(item);
    }
  
  return _attr_decoded;
}


void Url::update()
{
  QString q = "";
  
  // set the attributes to query
  if (_attributes.count() > 0)
    {
      q += "(";
      for (unsigned int i=0; i < _attributes.count()-1; ++i)
	{
	  q += _attributes.at(i);
	  q += ",";
	}
      q += _attributes.at(_attributes.count()-1);
      q += ")";
    }

  // set the scope
  q += "?";
  if (_scope == LDAP_SCOPE_SUBTREE)
    q += "sub";
  if (_scope == LDAP_SCOPE_ONELEVEL)
    q += "one";

  // set the filter
  if (_filter != "(objectClass=*)") 
     q += "?" + _filter;	
  else
     q += "?";
  
  // set the extensions
  q += "?" + _extensions;

  // remove trailing ´?´
  while (q.right(1) == "?")
    q.remove(q.length()-1,1);

  setQuery(q);
}
