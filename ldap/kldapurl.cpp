#include <lber.h>
#include <ldap.h>

#include "kldapurl.h"

#include <kdebug.h>
#include <qstringlist.h>

using namespace KLDAP;


Url::Url(const KURL &_url)
  : KURL(_url), _extensions("")
{
  parseLDAP();
}


void Url::parseLDAP()
{
  // extract the dn
  _dn = path();
  if (_dn.startsWith("/"))
    _dn.remove(0,1);  

  // parse the query  
  QString q = query();
  // remove first ?
  if (q.startsWith("?"))
    q.remove(0,1);

  // split into a list
  QStringList url_items = QStringList::split("?", q, true);
 
  // first come the attributes to query
  _attributes.clear();
  if (url_items.count() >= 1)
    {
      q = *url_items.at(0);
      if (q.startsWith("("))
	q.remove(0,1);
      if (q.endsWith(")"))
	q.remove(q.length()-1,1);
      if (!q.isEmpty())
	_attributes = QStringList::split(",", q, false);
    }

  // second the scope
  _scope = LDAP_SCOPE_BASE;
  if (url_items.count() >= 2)
    {
      if ((*url_items.at(1)).lower() == "sub")
	_scope = LDAP_SCOPE_SUBTREE;
      if ((*url_items.at(1)).lower() == "one")
	_scope = LDAP_SCOPE_ONELEVEL;
    }

  // third is simply the filter
  if (url_items.count() >= 3 )
    _filter = *url_items.at(2);
  if (_filter.isEmpty())
    _filter = "(objectClass=*)";

}


void Url::update()
{
  QString q = "?";
  
  // set the attributes to query
  if (_attributes.count() > 0)
      q += _attributes.join(",");
   else
      q += "*";	// all attributes

  // set the scope
  q += "?";
  if (_scope == LDAP_SCOPE_SUBTREE)
    q += "sub";
  else if (_scope == LDAP_SCOPE_ONELEVEL)
    q += "one";
  else if (_scope == LDAP_SCOPE_BASE)
    q += "base";

  q += "?";
  // set the filter
  if (_filter != "(objectClass=*)") 
     q += _filter;	
  
  // set the extensions
  q += "?" + _extensions;

  // remove trailing ´?´
  while (q.endsWith("?"))
    q.remove(q.length()-1,1);

  setQuery(q);
}
