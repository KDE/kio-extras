#include <qstring.h>

//extern char *encodeBase64(const char * src, unsigned long srcl, unsigned long & destl);
//extern char *decodeBase64(const QCString & s, unsigned long & len);

QString encodeBase64(const QString aStr, int* aLenPtr);
QString decodeBase64(const QString aStr, int* aLenPtr);

QString encodeMD5(const QString & src);
