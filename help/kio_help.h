#ifndef __help_h__
#define __help_h__ 


#include <sys/types.h>
#include <sys/stat.h>


#include <stdio.h>
#include <unistd.h>


#include <qintdict.h>
#include <qstring.h>
#include <qvaluelist.h>


#include <kio/global.h>
#include <kio/slavebase.h>


class HelpProtocol : public KIO::SlaveBase
{
public:

  HelpProtocol( const QCString &pool, const QCString &app);
  virtual ~HelpProtocol() { }

  virtual void get( const KURL& url, bool reload );

  virtual void mimetype( const KURL& url );


private:

  QString langLookup(QString fname);
  
  QString lookupFile(QString fname, QString query);

};


#endif
