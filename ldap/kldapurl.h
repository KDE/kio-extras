#ifndef _K_LDAPURL_H_
#define _K_LDAPURL_H_

#include <qstring.h>
#include <qstringlist.h>

#include <kurl.h>

namespace KLDAP
{

  class Url : public KURL
  {
  public:

    Url(const KURL &_url);
  
    QString dn() const { return _dn; };
    void setDn(QString dn) { _dn = dn; update(); };

    QStringList &attributes() { return _attributes; };
    void setAttributes(const QStringList &attributes) { _attributes=attributes; update(); };

    int scope() const { return _scope; };
    void setScope(int scope) { _scope=scope; update(); };

    QString filter() const { QString item=decode_string(_filter); return item; };
    void setFilter(QString filter) { _filter=filter; update(); } ;

    QString bindDN() const { return _bindDN; }
    void setBindDN( const QString &bindDN ) { _bindDN = bindDN; }

    QString pwdBindDN() const { return _pwdBindDN; }
    void setPwdBindDN( const QString &pwdBindDN ) { _pwdBindDN = pwdBindDN; }

  protected:

    void parseLDAP();
    void update();

  private:

    QString  _dn;
    QStringList _attributes;
    int      _scope;
    QString  _filter;
    QString  _extensions;
    QString  _bindDN;
    QString  _pwdBindDN;
  };
};

#endif
