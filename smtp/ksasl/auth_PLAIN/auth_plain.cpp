// $Id$

#include <stdio.h>
#include <stdlib.h>

#include <qstring.h>

#include <kdebug.h>

#include "../saslmodule.h"

class PlainAuth
	: public KSASLAuthModule
{
public:
	PlainAuth();
	virtual ~PlainAuth();
	virtual QString auth_method();
	virtual QString auth_response(const QString &challenge, const KURL &auth_url);
};

extern "C" {
	KSASLAuthModule *auth_init()
	{
		return new PlainAuth;
	}
	int module_version() {return KSASL_MODULE_REV;}
}

QString base64_encode_string(const char *string, unsigned int len);
QString base64_encode_auth_line(const char *username, const char *pass);

PlainAuth::PlainAuth()
	: KSASLAuthModule()
{
}

PlainAuth::~PlainAuth()
{
}

QString PlainAuth::auth_method()
{
	return QString("PLAIN");
}

QString PlainAuth::auth_response(const QString &, const KURL &auth_url)
{
	return base64_encode_auth_line(auth_url.user().latin1(), auth_url.pass().latin1());
}

/*
 * Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
 *
 * Permission to use, copy, modify, and distribute this material
 * for any purpose and without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies, and that the name of Bellcore not be
 * used in advertising or publicity pertaining to this
 * material without the specific, prior written permission
 * of an authorized representative of Bellcore.  BELLCORE
 * MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
 * OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
 */
QString base64_encode_string( const char *buf, unsigned int len )
{
  char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  QString out;
  int inPos  = 0;
  int c1, c2, c3;
  unsigned int i;

  /* Get three characters at a time and encode them. */
  for (i=0; i < len/3; ++i) {
      c1 = buf[inPos++] & 0xFF;
      c2 = buf[inPos++] & 0xFF;
      c3 = buf[inPos++] & 0xFF;
      out+= basis_64[(c1 & 0xFC) >> 2];
      out+= basis_64[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)];
      out+= basis_64[((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6)];
      out+= basis_64[c3 & 0x3F];
  }

  /* Encode the remaining one or two characters. */

  switch (len % 3) {
      case 0:
          break;
      case 1:
          c1 = buf[inPos] & 0xFF;
          out+= basis_64[(c1 & 0xFC) >> 2];
          out+= basis_64[((c1 & 0x03) << 4)];
          out+= '=';
          out+= '=';
          break;
      case 2:
          c1 = buf[inPos++] & 0xFF;
          c2 = buf[inPos] & 0xFF;
          out+= basis_64[(c1 & 0xFC) >> 2];
          out+= basis_64[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)];
          out+= basis_64[((c2 & 0x0F) << 2)];
          out+= '=';
          break;
  }
  return out.copy();
}

QString base64_encode_auth_line(const char *username, const char *pass)
{
	QString ret;
        int len;
        char *line;
        len=((strlen(username)*2)+strlen(pass)+2);
        line=static_cast<char *>(malloc(len));
        sprintf(line, "%s%c%s%c%s", username, 0, username,0, pass);
        ret=base64_encode_string(line, len);
        free(line);
        return ret;
}
