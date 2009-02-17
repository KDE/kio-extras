/* Copytester is a test case for the fish kioslave. It copies 100 files from /tmp to /tmp/test using fish's functions.
It was written to verify KDE bug 147948: https://bugs.kde.org/show_bug.cgi?id=147948

To use this program, do
 mkdir /tmp/test
 for i in $(seq 1 1 100); do touch /tmp/fishtest${i}.txt; done
*/

#include <kio/scheduler.h>
#include <kurl.h>
#include <kio/jobclasses.h>
#include <kdebug.h>
#include <copytester.h>
#include <kio/copyjob.h>
#include <kapplication.h> 

class TransferJob;

Browser::Browser() : QWidget(NULL)
{
  slotButtonClicked();
}

void Browser::slotButtonClicked()
{
  kDebug() << "entering function";
  // creating a kioslave
  kDebug() << "getting via fish*************************************************************";
  KUrl::List selectedUrls;

  for (int i=1; i<=99; i++)
  {
    QString filename=QString("/tmp/fishtest");
    filename.append(QString::number(i)).append(".txt");
    kDebug() << filename;
    selectedUrls.push_back(KUrl(filename));
  }
  KUrl destUrl("fish://root@localhost/tmp/test");
  KIO::CopyJob* job0 = KIO::copy( selectedUrls, destUrl );
  job0->start();
}

void Browser::dataishere(KIO::Job *,const QByteArray & data )
{
  static int counter=0;
  kDebug() << ++counter << " data is here*************************************************************";
  kDebug() << data;
  kapp->quit();
}
