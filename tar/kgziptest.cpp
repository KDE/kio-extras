#include "kgzipdev.h"
#include <qfile.h>
#include <kdebug.h>

int main()
{
    QFile f("./test.gz");
    KGzipDev dev(&f);
    dev.open( IO_ReadOnly );

    // This is what KGzipDev::readAll could do, if QIODevice::readAll was virtual....

    QByteArray array(1024);
    int n;
    while ( ( n = dev.readBlock( array.data(), array.size() ) ) )
    {
        kdDebug() << "readBlock returned " << n << endl << endl;
        // QCString s(array,n+1); // Terminate with 0 before printing
        // printf("%s", s.data());

        kdDebug() << "dev.at = " << dev.at() << endl;
        kdDebug() << "f.at = " << f.at() << endl;
    }
    dev.close();

   {
    QFile f("./test.gz");
    KGzipDev dev(&f);
    dev.open( IO_ReadOnly );
    int ch;
    while ( ( ch = dev.getch() ) != -1 )
      printf("%c",ch);
    dev.close();
   }

    return 0;
}
