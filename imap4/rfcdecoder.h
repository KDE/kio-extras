#ifndef RFCDECODER_H
#define RFCDECODER_H
/**********************************************************************
 *
 *   rfcdecoder.h  - handler for various rfc/mime encodings
 *   Copyright (C) 2000 s.carstens@gmx.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   Send comments and bug fixes to s.carstens@gmx.de
 *
 *********************************************************************/

#include <qstring.h>

class QTextCodec;

/**
 * handler for various rfc/mime encodings
 * @author Sven Carstens <s.carstens@gmx.de>
 * @date 2000
 * @todo rename to rfcCodecs as it encodes too.
 */
class rfcDecoder
{

public:

/** Convert an IMAP mailbox to a Unicode path
 */
  static QString fromIMAP (const QString & src);
/** Convert Unicode path to modified UTF-7 IMAP mailbox
 */
  static QString toIMAP (const QString & inSrc);
/** replace " with \" and \ with \\ " and \ characters */
  static QString quoteIMAP (const QString & src);

  /** remove \ from a string 
   * @bug I'm pretty sure this doesn't do what the author meant it to do
   */
  static QString decodeQuoting(const QString &aStr);

  /**
   * fetch a codec by name
   * @return Text Codec object
   */
  static QTextCodec *codecForName (const QString &);

  // decoder for RFC2047 and RFC1522
  /** decode a RFC2047 String */
  static const QString decodeRFC2047String (const QString & _str,
                                            QString & charset,
                                            QString & language);
  /** decode a RFC2047 String */
  static const QString decodeRFC2047String (const QString & _str,
                                            QString & charset);
  /** decode a RFC2047 String */
  static const QString decodeRFC2047String (const QString & _str);

  // encoder for RFC2047 and RFC1522
  /** encode a RFC2047 String */
  static const QString encodeRFC2047String (const QString & _str,
                                            QString & charset,
                                            QString & language);
  /** encode a RFC2047 String */
  static const QString encodeRFC2047String (const QString & _str,
                                            QString & charset);
  /** encode a RFC2047 String */
  static const QString encodeRFC2047String (const QString & _str);

  /** encode a RFC2231 String */
  static const QString encodeRFC2231String (const QString & _str);
  /** decode a RFC2231 String */
  static const QString decodeRFC2231String (const QString & _str);
};

#endif
