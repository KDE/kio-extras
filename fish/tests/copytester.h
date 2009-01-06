/* Copytester is a test case for the fish kioslave. It copies 100 files from /tmp to /tmp/test using fish's functions.
It was written to verify KDE bug 147948: https://bugs.kde.org/show_bug.cgi?id=147948 */

#ifndef KDE4START_H__
#define KDE4START_H__

#include <kmainwindow.h>
#include <kio/scheduler.h>
#include <kurl.h>
#include <kio/jobclasses.h>

class Browser : public QWidget
{
  Q_OBJECT
  public:
    Browser();
  public slots:
    void slotButtonClicked();
    void dataishere(KIO::Job *,const QByteArray &);
};
#endif
