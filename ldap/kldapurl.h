#ifndef _K_LDAPURL_H_
#define _K_LDAPURL_H_

#include <kurl.h>
#include <qstring.h>
#include <qstringlist.h>


namespace KLDAP
{

  class Url : public KURL
  {
  public:

    Url(const KURL &_url);
  
    QString dn() { return _dn; };
    void setDn(QString dn) { _dn = dn; update(); };

    QStringList &attributes() { return _attributes; };
    void setAttributes(const QStringList &attributes) { _attributes=attributes; update(); };

    int scope() { return _scope; };
    void setScope(int scope) { _scope=scope; update(); };

    QString filter() { QString item=decode_string(_filter); return item; };
    void setFilter(QString filter) { _filter=filter; update(); } ;

  protected:

    void parseLDAP();
    void update();

  private:

    QString  _dn;
    QStringList _attributes;
    int      _scope;
    QString  _filter;
    QString  _extensions;

  };
};

#endif
