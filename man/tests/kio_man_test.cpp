

#include <QObject>

#include "kio_man.h"


#include <QApplication>
#include <KLocalizedString>


class kio_man_test : public  MANProtocol
{
    Q_OBJECT

public:
    kio_man_test(const QByteArray &pool_socket, const QByteArray &app_socket);

};





int main(int argc, char **argv)
{
    QApplication a( argc, argv);

    MANProtocol testproto("/tmp/kiotest.in", "/tmp/kiotest.out");
    testproto.showIndex("3");

    return 0;
}

#include "kio_man_test.moc"
