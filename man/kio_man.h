#ifndef __kio_man_h__
#define __kio_man_h__


#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <qdict.h>


#include <kio/global.h>
#include <kio/slavebase.h>


class KProcess;


class MANProtocol : public QObject, public KIO::SlaveBase
{
  Q_OBJECT

public:

  MANProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~MANProtocol();

  virtual void get(const QString& path, const QString& query, bool reload);
  virtual void stat(const QString& path, const QString& query);

  virtual void mimetype(const QString &path, const QString& query);

  void outputError(QString errmsg);


  void showMainIndex();
  void showIndex(QString section);


private slots:

  void shellStdout(KProcess *, char *buffer, int buflen);


private:

  void initCache(QString section);
  QString pageName(QString page);

  QDict<char> *_cache;


  QCString _shellStdout;

};


#endif
