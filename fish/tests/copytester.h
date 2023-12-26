/* Copytester is a test case for the fish KIO worker. It copies 100 files from /tmp to /tmp/test using fish's functions.
It was written to verify KDE bug 147948: https://bugs.kde.org/show_bug.cgi?id=147948 */

#ifndef KDE4START_H__
#define KDE4START_H__

#include <KIO/Job>
#include <kmainwindow.h>
#include <kurl.h>

class Browser : public QWidget
{
    Q_OBJECT
public:
    Browser();
public Q_SLOTS:
    void slotButtonClicked();
    void dataishere(KIO::Job *, const QByteArray &);
};
#endif
