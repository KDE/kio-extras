// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <qstring.h>

/* decode file as MIME base64 (RFC 1341) by John Walker http://www.fourmilab.ch/ This program is in the public domain. */
typedef unsigned char byte;	      /* Byte type */
static byte dtable[256];	      /* Encode / decode table */
static int errcheck = TRUE;	      /* Check decode input for errors ? */

QString base64_decode_string(const char *string, unsigned int len)
{
    QString ret;
    unsigned int i,z=0;

    for (i = 0; i < 255; i++) {
	dtable[i] = 0x80;
    }
    for (i = 'A'; i <= 'Z'; i++) {
        dtable[i] = 0 + (i - 'A');
    }
    for (i = 'a'; i <= 'z'; i++) {
        dtable[i] = 26 + (i - 'a');
    }
    for (i = '0'; i <= '9'; i++) {
        dtable[i] = 52 + (i - '0');
    }
    dtable['+'] = 62;
    dtable['/'] = 63;
    dtable['='] = 0;

    /*CONSTANTCONDITION*/
    while (TRUE) {
	byte a[4], b[4], o[3];

	for (i = 0; i < 4; i++) {
	    int c;

	    if (z >= len) {
		if (errcheck && (i > 0)) {
                    fprintf(stderr, "Input file incomplete.\n");
		    return QString::null;
		}
		return ret;
	    }
	    c=string[z]; z++;
	    if (dtable[c] & 0x80) {
		if (errcheck) {
                    fprintf(stderr, "Illegal character '%c' in input file.\n", c);
		    return QString::null;
		}
		/* Ignoring errors: discard invalid character. */
		i--;
		continue;
	    }
	    a[i] = (byte) c;
	    b[i] = (byte) dtable[c];
	}
	o[0] = (b[0] << 2) | (b[1] >> 4);
	o[1] = (b[1] << 4) | (b[2] >> 2);
	o[2] = (b[2] << 6) | b[3];
        i = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);
	for (unsigned int w=0; w < i; w++)
		ret.append(o[w]);
	if (i < 3) {
	    return ret;
	}
    }
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

QString create_auth_plain(const char *username, const char *pass)
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
