#include <qstring.h>

// Set up C++ wrappers for base64 and MD5
const QString encodeBase64(const QString aStr, int* aLenPtr);
const QString decodeBase64(const QString aStr, int* aLenPtr);

const QString encodeMD5(const QString & src);

