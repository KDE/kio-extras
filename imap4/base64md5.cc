
#include <qstring.h>
#include "base64.cc"
#include "md5.h"

QString encodeMD5(const QString & src) {
  MD5_CTX context;
  unsigned char md5text[16];
  QString result;

  MD5Init(&context);
  MD5Update(&context, (char *)src.ascii(), src.length());
  MD5Final(md5text, &context);
  QString h = "0123456789abcdef";
  for(int i=0; i<16; i++) {
    int c = md5text[i];
    result += h[(c >> 4) & 0xF];
    result += h[c & 0xF];
  }
  return QString(result);
}

