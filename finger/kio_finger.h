#ifndef __kio_finger_h__
#define __kio_finger_h__

#include <qstring.h>
#include <qcstring.h>

#include <kurl.h>
#include <kprocess.h>
#include <kio/global.h>
#include <kio/slavebase.h>

class FingerProtocol : public QObject, public KIO::SlaveBase
{
  Q_OBJECT

public:

  FingerProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~FingerProtocol();

  virtual void get(const KURL& url);
  virtual void mimetype(const KURL &url);
  virtual void listDir(const KURL &url);

private slots:
  void       slotGetStdOutput(KProcess*, char*, int);

private:
  QString	        *myPerlPath; 		
  QString               *myFingerScript; 
  QString		*myHTMLHeader;
  QString		*myHTMLTail;
  QString		*myStdStream;  

  KURL                  *myURL;

  KShellProcess	        *myKProcess;

  void       getProgramPath();
  void       parseCommandLine(const KURL& url);
};


#endif
