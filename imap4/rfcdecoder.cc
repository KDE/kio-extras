/**********************************************************************
 *
 *   rfcdecoder.cc  - handler for various rfc/mime encodings
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Send comments and bug fixes to s.carstens@gmx.de
 *
 *********************************************************************/
#include "rfcdecoder.h"
#include "md5.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>

#include <qtextcodec.h>
#include <qbuffer.h>
#include <qregexp.h>

// This part taken from rfc 2192 IMAP URL Scheme. C. Newman. September 1997.
// adapted to QT-Toolkit by Sven Carstens <s.carstens@gmx.de> 2000

static char base64chars[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";
#define UNDEFINED 64
#define MAXLINE  76

/* UTF16 definitions */
#define UTF16MASK       0x03FFUL
#define UTF16SHIFT      10
#define UTF16BASE       0x10000UL
#define UTF16HIGHSTART  0xD800UL
#define UTF16HIGHEND    0xDBFFUL
#define UTF16LOSTART    0xDC00UL
#define UTF16LOEND      0xDFFFUL

/* Convert an IMAP mailbox to a Unicode path
 */
const QString
rfcDecoder::fromIMAP (const QString & inSrc)
{
  unsigned char c, i, bitcount;
  unsigned long ucs4, utf16, bitbuf;
  unsigned char base64[256], utf8[6];
  unsigned long srcPtr = 0;
  QCString dst;
  QCString src = inSrc.ascii ();

  /* initialize modified base64 decoding table */
  memset (base64, UNDEFINED, sizeof (base64));
  for (i = 0; i < sizeof (base64chars); ++i)
  {
    base64[base64chars[i]] = i;
  }

  /* loop until end of string */
  while (srcPtr < src.length ())
  {
    c = src[srcPtr++];
    /* deal with literal characters and &- */
    if (c != '&' || *src == '-')
    {
      /* encode literally */
      dst += c;
      /* skip over the '-' if this is an &- sequence */
      if (c == '&')
        srcPtr++;
    }
    else
    {
      /* convert modified UTF-7 -> UTF-16 -> UCS-4 -> UTF-8 -> HEX */
      bitbuf = 0;
      bitcount = 0;
      ucs4 = 0;
      while ((c = base64[(unsigned char) src[srcPtr]]) != UNDEFINED)
      {
        ++srcPtr;
        bitbuf = (bitbuf << 6) | c;
        bitcount += 6;
        /* enough bits for a UTF-16 character? */
        if (bitcount >= 16)
        {
          bitcount -= 16;
          utf16 = (bitcount ? bitbuf >> bitcount : bitbuf) & 0xffff;
          /* convert UTF16 to UCS4 */
          if (utf16 >= UTF16HIGHSTART && utf16 <= UTF16HIGHEND)
          {
            ucs4 = (utf16 - UTF16HIGHSTART) << UTF16SHIFT;
            continue;
          }
          else if (utf16 >= UTF16LOSTART && utf16 <= UTF16LOEND)
          {
            ucs4 += utf16 - UTF16LOSTART + UTF16BASE;
          }
          else
          {
            ucs4 = utf16;
          }
          /* convert UTF-16 range of UCS4 to UTF-8 */
          if (ucs4 <= 0x7fUL)
          {
            utf8[0] = ucs4;
            i = 1;
          }
          else if (ucs4 <= 0x7ffUL)
          {
            utf8[0] = 0xc0 | (ucs4 >> 6);
            utf8[1] = 0x80 | (ucs4 & 0x3f);
            i = 2;
          }
          else if (ucs4 <= 0xffffUL)
          {
            utf8[0] = 0xe0 | (ucs4 >> 12);
            utf8[1] = 0x80 | ((ucs4 >> 6) & 0x3f);
            utf8[2] = 0x80 | (ucs4 & 0x3f);
            i = 3;
          }
          else
          {
            utf8[0] = 0xf0 | (ucs4 >> 18);
            utf8[1] = 0x80 | ((ucs4 >> 12) & 0x3f);
            utf8[2] = 0x80 | ((ucs4 >> 6) & 0x3f);
            utf8[3] = 0x80 | (ucs4 & 0x3f);
            i = 4;
          }
          /* copy it */
          for (c = 0; c < i; ++c)
          {
            dst += utf8[c];
          }
        }
      }
      /* skip over trailing '-' in modified UTF-7 encoding */
      if (src[srcPtr] == '-')
        ++srcPtr;
    }
  }
  return QString::fromUtf8 (dst.data ());
}

/* Convert Unicode path to modified UTF-7 IMAP mailbox
 */
const QString
rfcDecoder::toIMAP (const QString & inSrc)
{
  unsigned int utf8pos, utf8total, c, utf7mode, bitstogo, utf16flag;
  unsigned long ucs4, bitbuf;
  QCString src = inSrc.utf8 ();
  QString dst;

  ulong srcPtr = 0;
  utf7mode = 0;
  utf8total = 0;
  bitstogo = 0;
  utf8pos = 0;
  bitbuf = 0;
  ucs4 = 0;
  while (srcPtr < src.length ())
  {
    c = (unsigned char) src[srcPtr++];
    /* normal character? */
    if (c >= ' ' && c <= '~')
    {
      /* switch out of UTF-7 mode */
      if (utf7mode)
      {
        if (bitstogo)
        {
          dst += base64chars[(bitbuf << (6 - bitstogo)) & 0x3F];
        }
        dst += '-';
        utf7mode = 0;
      }
      dst += c;
      /* encode '&' as '&-' */
      if (c == '&')
      {
        dst += '-';
      }
      continue;
    }
    /* switch to UTF-7 mode */
    if (!utf7mode)
    {
      dst += '&';
      utf7mode = 1;
    }
    /* Encode US-ASCII characters as themselves */
    if (c < 0x80)
    {
      ucs4 = c;
      utf8total = 1;
    }
    else if (utf8total)
    {
      /* save UTF8 bits into UCS4 */
      ucs4 = (ucs4 << 6) | (c & 0x3FUL);
      if (++utf8pos < utf8total)
      {
        continue;
      }
    }
    else
    {
      utf8pos = 1;
      if (c < 0xE0)
      {
        utf8total = 2;
        ucs4 = c & 0x1F;
      }
      else if (c < 0xF0)
      {
        utf8total = 3;
        ucs4 = c & 0x0F;
      }
      else
      {
        /* NOTE: can't convert UTF8 sequences longer than 4 */
        utf8total = 4;
        ucs4 = c & 0x03;
      }
      continue;
    }
    /* loop to split ucs4 into two utf16 chars if necessary */
    utf8total = 0;
    do
    {
      if (ucs4 >= UTF16BASE)
      {
        ucs4 -= UTF16BASE;
        bitbuf = (bitbuf << 16) | ((ucs4 >> UTF16SHIFT) + UTF16HIGHSTART);
        ucs4 = (ucs4 & UTF16MASK) + UTF16LOSTART;
        utf16flag = 1;
      }
      else
      {
        bitbuf = (bitbuf << 16) | ucs4;
        utf16flag = 0;
      }
      bitstogo += 16;
      /* spew out base64 */
      while (bitstogo >= 6)
      {
        bitstogo -= 6;
        dst += base64chars[(bitstogo ? (bitbuf >> bitstogo) : bitbuf) & 0x3F];
      }
    }
    while (utf16flag);
  }
  /* if in UTF-7 mode, finish in ASCII */
  if (utf7mode)
  {
    if (bitstogo)
    {
      dst += base64chars[(bitbuf << (6 - bitstogo)) & 0x3F];
    }
    dst += '-';
  }
  return dst;
}

//-----------------------------------------------------------------------------
QTextCodec *
rfcDecoder::codecForName (const QString & _str)
{
  if (_str.isEmpty ())
    return NULL;
  return QTextCodec::codecForName (_str.lower ().
                                   replace (QRegExp ("windows"),
                                            "cp").latin1 ());
}

//-----------------------------------------------------------------------------
const QString
rfcDecoder::decodeRFC2047String (const QString & _str)
{
  QString throw_away;

  return decodeRFC2047String (_str, throw_away);
}

//-----------------------------------------------------------------------------
const QString
rfcDecoder::decodeRFC2047String (const QString & _str, QString & charset)
{
  QString throw_away;

  return decodeRFC2047String (_str, charset, throw_away);
}

//-----------------------------------------------------------------------------
const QString
rfcDecoder::decodeRFC2047String (const QString & _str, QString & charset,
                                 QString & language)
{
  QCString aStr = _str.ascii ();  // QString.length() means Unicode chars
  QCString result;
  char *pos, *beg, *end, *mid;
  QCString str;
  char encoding, ch;
  bool valid;
  const int maxLen = 200;
  int i;

  //do we have a rfc string
  if (aStr.find ("=?") < 0)
    return aStr;

//  result.truncate(aStr.length());
  for (pos = aStr.data (); *pos; pos++)
  {
    if (pos[0] != '=' || pos[1] != '?')
    {
      result += *pos;
      continue;
    }
    beg = pos + 2;
    end = beg;
    valid = TRUE;
    // parse charset name
    for (i = 2, pos += 2;
         i < maxLen && (*pos != '?' && (ispunct (*pos) || isalnum (*pos)));
         i++)
      pos++;
    if (*pos != '?' || i < 4 || i >= maxLen)
      valid = FALSE;
    else
    {
      charset = QCString (beg, i - 1);  // -2 + 1 for the zero
      if (charset.findRev ('*') != -1)
      {
        // save language for later usage
        language =
          charset.right (charset.length () - charset.findRev ('*') - 1);

        // tie off language as defined in rfc2047
        charset = charset.left (charset.findRev ('*'));
      }
      // get encoding and check delimiting question marks
      encoding = toupper (pos[1]);
      if (pos[2] != '?'
          || (encoding != 'Q' && encoding != 'B' && encoding != 'q'
              && encoding != 'b'))
        valid = FALSE;
      pos += 3;
      i += 3;
//    qDebug("rfcDecoder::decodeRFC2047String - charset %s - language %s - '%s'",charset.latin1(),language.latin1(),pos);
    }
    if (valid)
    {
      mid = pos;
      // search for end of encoded part
      while (i < maxLen && *pos && !(*pos == '?' && *(pos + 1) == '='))
      {
        i++;
        pos++;
      }
      end = pos + 2;            //end now points to the first char after the encoded string
      if (i >= maxLen || !*pos)
        valid = FALSE;
    }
    if (valid)
    {
      ch = *pos;
      *pos = '\0';
      str = QCString (mid).left ((int) (mid - pos - 1));
      if (encoding == 'Q')
      {
        // decode quoted printable text
        for (i = str.length () - 1; i >= 0; i--)
          if (str[i] == '_')
            str[i] = ' ';
//    qDebug("rfcDecoder::decodeRFC2047String - before QP '%s'",str.data());

        str = QCString (decodeQuotedPrintable (str));
//    qDebug("rfcDecoder::decodeRFC2047String - after QP '%s'",str.data());
      }
      else
      {
        // decode base64 text
        str = QCString (decodeBase64 (str));
      }
      *pos = ch;
      for (i = 0; i < (int) str.length (); i++)
        result += (char) (QChar) str[i];

      pos = end - 1;
    }
    else
    {
//    qDebug("rfcDecoder::decodeRFC2047String - invalid");
      //result += "=?";
      //pos = beg -1; // because pos gets increased shortly afterwards
      pos = beg - 2;
      result += *pos++;
      result += *pos;
    }
  }
  if (!charset.isEmpty ())
  {
    QTextCodec *aCodec = codecForName (charset.ascii ());
    if (aCodec)
    {
//    qDebug("Codec is %s",aCodec->name());
      return aCodec->toUnicode (result);
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
const char especials[17] = "()<>@,;:\"/[]?.= ";

const QString
rfcDecoder::encodeRFC2047String (const QString & _str)
{
  if (_str.isEmpty ())
    return _str;
  signed char *latin = (signed char *) calloc (1, _str.length () + 1);
  char *latin_un = (char *) latin;
  strcpy (latin_un, _str.latin1 ());
  signed char *latinStart = latin, *l, *start, *stop;
  char hexcode;
  int numQuotes, i;
  QCString result;
  while (*latin)
  {
    l = latin;
    start = latin;
    while (*l)
    {
      if (*l == 32)
        start = l + 1;
      if (*l < 0)
        break;
      l++;
    }
    if (*l)
    {
      numQuotes = 1;
      while (*l)
      {
        /* The encoded word must be limited to 75 character */
        for (i = 0; i < 16; i++)
          if (*l == especials[i])
            numQuotes++;
        if (*l < 0)
          numQuotes++;
        /* Stop after 58 = 75 - 17 characters or at "<user@host..." */
        if (l - start + 2 * numQuotes >= 58 || *l == 60)
          break;
        l++;
      }
      if (*l)
      {
        stop = l - 1;
        while (stop >= start && *stop != 32)
          stop--;
        if (stop <= start)
          stop = l;
      }
      else
        stop = l;
      while (latin < start)
      {
        result += *latin;
        latin++;
      }
      result += QCString ("=?iso-8859-1?q?");
      while (latin < stop)
      {
        numQuotes = 0;
        for (i = 0; i < 16; i++)
          if (*latin == especials[i])
            numQuotes = 1;
        if (*latin < 0)
          numQuotes = 1;
        if (numQuotes)
        {
          result += "=";
          hexcode = ((*latin & 0xF0) >> 4) + 48;
          if (hexcode >= 58)
            hexcode += 7;
          result += hexcode;
          hexcode = (*latin & 0x0F) + 48;
          if (hexcode >= 58)
            hexcode += 7;
          result += hexcode;
        }
        else
        {
          result += *latin;
        }
        latin++;
      }
      result += "?=";
    }
    else
    {
      while (*latin)
      {
        result += *latin;
        latin++;
      }
    }
  }
  free (latinStart);
  return result;
}


//-----------------------------------------------------------------------------
const QString
rfcDecoder::encodeRFC2231String (const QString & _str)
{
  if (_str.isEmpty ())
    return _str;
  signed char *latin = (signed char *) calloc (1, _str.length () + 1);
  char *latin_us = (char *) latin;
  strcpy (latin_us, _str.latin1 ());
  signed char *l = latin;
  char hexcode;
  int i;
  bool quote;
  while (*l)
  {
    if (*l < 0)
      break;
    l++;
  }
  if (!*l)
    return _str.ascii ();
  QCString result;
  l = latin;
  while (*l)
  {
    quote = *l < 0;
    for (i = 0; i < 16; i++)
      if (*l == especials[i])
        quote = true;
    if (quote)
    {
      result += "%";
      hexcode = ((*l & 0xF0) >> 4) + 48;
      if (hexcode >= 58)
        hexcode += 7;
      result += hexcode;
      hexcode = (*l & 0x0F) + 48;
      if (hexcode >= 58)
        hexcode += 7;
      result += hexcode;
    }
    else
    {
      result += *l;
    }
    l++;
  }
  free (latin);
  return result;
}


//-----------------------------------------------------------------------------
const QString
rfcDecoder::decodeRFC2231String (const QString & _str)
{
  QString charset;
  QString language;

  int p = _str.find ("'");
  int l = _str.findRev ("'");

  //see if it is an rfc string
  if (p < 0)
    return _str;

  //first is charset or empty
  charset = _str.left (p);
  QString st = _str.mid (l + 1);

  //second is language
  if (p >= l)
    return _str;
  language = _str.mid (p + 1, l - p - 1);

  //qDebug("Charset: %s Language: %s",charset.latin1(),language.latin1());

  char ch, ch2;
  p = 0;
  while (p < (int) st.length ())
  {
    if (st.at (p) == 37)
    {
      ch = st.at (p + 1).latin1 () - 48;
      if (ch > 16)
        ch -= 7;
      ch2 = st.at (p + 2).latin1 () - 48;
      if (ch2 > 16)
        ch2 -= 7;
      st.at (p) = ch * 16 + ch2;
      st.remove (p + 1, 2);
    }
    p++;
  }
  return st;
}

//=============================================================================
// Code below is adapted from:
//
// Copyright (c) 1996, 1997 Douglas W. Sauder
// All rights reserved.
// 
// IN NO EVENT SHALL DOUGLAS W. SAUDER BE LIABLE TO ANY PARTY FOR DIRECT,
// INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF
// THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF DOUGLAS W. SAUDER
// HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// DOUGLAS W. SAUDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
// BASIS, AND DOUGLAS W. SAUDER HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
// SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
//
//=============================================================================

static char base64tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz0123456789+/";

static char base64idx[128] = {
  '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', '\377', '\377', 62, '\377', '\377', '\377', 63,
  52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, '\377', '\377', '\377', '\377', '\377', '\377',
  '\377', 0, 1, 2, 3, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, '\377', '\377', '\377', '\377', '\377',
  '\377', 26, 27, 28, 29, 30, 31, 32,
  33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, '\377', '\377', '\377', '\377', '\377'
};

static char hextab[] = "0123456789ABCDEF";

inline int
isbase64 (int a)
{
  return ('A' <= a && a <= 'Z')
    || ('a' <= a && a <= 'z')
    || ('0' <= a && a <= '9') || a == '+' || a == '/';
}

const QCString
rfcDecoder::decodeBase64 (const QCString & _str)
{
  QByteArray pass, get;
  QCString retVal;

  pass.setRawData (_str.data (), _str.length ());
  get = decodeBase64 (pass);
  pass.resetRawData (_str.data (), _str.length ());
  retVal = QCString (get.data (), get.count () + 1);
  return retVal;
};

const QCString
rfcDecoder::encodeBase64 (const QCString & _str)
{
  QByteArray pass, get;
  QCString retVal;

  pass.setRawData (_str.data (), _str.length ());
  get = encodeBase64 (pass);
  pass.resetRawData (_str.data (), _str.length ());
  retVal = QCString (get.data (), get.count () + 1);
  return retVal;
};

const QByteArray
rfcDecoder::encodeBase64 (const QByteArray & input)
{
  QByteArray output;
  QBuffer outStream (output);
  outStream.open (IO_WriteOnly);

  size_t inLen = input.count ();

  size_t inPos = 0;
  int c1, c2, c3;
  int lineLen = 0;

//  qDebug("rfcDecoder::encodeBase64 - got %d",inLen);

  /* Get three characters at a time and encode them. */
  for (size_t i = 0; i < inLen / 3; ++i)
  {
    c1 = input[inPos++] & 0xFF;
    c2 = input[inPos++] & 0xFF;
    c3 = input[inPos++] & 0xFF;
    outStream.putch (base64tab[(c1 & 0xFC) >> 2]);
    outStream.putch (base64tab[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)]);
    outStream.putch (base64tab[((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6)]);
    outStream.putch (base64tab[c3 & 0x3F]);
    lineLen += 4;
    if (lineLen >= MAXLINE - 3)
    {
      outStream.putch ('\r');
      outStream.putch ('\n');
      lineLen = 0;
    }
  }

  /* Encode the remaining one or two characters. */
  switch (inLen % 3)
  {
  case 0:
//    outStream.putch('=');
    outStream.putch ('\r');
    outStream.putch ('\n');
    break;
  case 1:
    c1 = input[inPos] & 0xFF;
    outStream.putch (base64tab[(c1 & 0xFC) >> 2]);
    outStream.putch (base64tab[((c1 & 0x03) << 4)]);
    outStream.putch ('=');
    outStream.putch ('=');
    outStream.putch ('\r');
    outStream.putch ('\n');
    break;
  case 2:
    c1 = input[inPos++] & 0xFF;
    c2 = input[inPos] & 0xFF;
    outStream.putch (base64tab[(c1 & 0xFC) >> 2]);
    outStream.putch (base64tab[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)]);
    outStream.putch (base64tab[((c2 & 0x0F) << 2)]);
    outStream.putch ('=');
    outStream.putch ('\r');
    outStream.putch ('\n');
    break;
  }
  QCString deb (output.data (), output.count () + 1);

//  qDebug("rfcDecoder::encodeBase64 - give %d '%s'",output.count(),deb.data());

  return output;
}


const QByteArray
rfcDecoder::decodeBase64 (const QByteArray & input)
{
  /* Get four input chars at a time and decode them. Ignore white space
   * chars (CR, LF, SP, HT). If '=' is encountered, terminate input. If
   * a char other than white space, base64 char, or '=' is encountered,
   * flag an input error, but otherwise ignore the char.
   */

  QByteArray output;
  QBuffer outStream (output);
  outStream.open (IO_ReadWrite);

  int isErr = 0;
  int isEndSeen = 0;
  unsigned char b1, b2, b3;
  unsigned char a1, a2, a3, a4;
  size_t inPos = 0;
  size_t inLen = input.count ();

//  qDebug("rfcDecoder::decodeBase64 - got %d",inLen);

  while (inPos < inLen)
  {
    a1 = a2 = a3 = a4 = 0;
    while (inPos < inLen)
    {
      a1 = input[inPos++] & 0xFF;
      if (isbase64 (a1))
      {
        break;
      }
      else if (a1 == '=')
      {
        isEndSeen = 1;
        break;
      }
      else if (a1 != '\r' && a1 != '\n' && a1 != ' ' && a1 != '\t')
      {
        isErr = 1;
      }
    }
    while (inPos < inLen)
    {
      a2 = input[inPos++] & 0xFF;
      if (isbase64 (a2))
      {
        break;
      }
      else if (a2 == '=')
      {
        isEndSeen = 1;
        break;
      }
      else if (a2 != '\r' && a2 != '\n' && a2 != ' ' && a2 != '\t')
      {
        isErr = 1;
      }
    }
    while (inPos < inLen)
    {
      a3 = input[inPos++] & 0xFF;
      if (isbase64 (a3))
      {
        break;
      }
      else if (a3 == '=')
      {
        isEndSeen = 1;
        break;
      }
      else if (a3 != '\r' && a3 != '\n' && a3 != ' ' && a3 != '\t')
      {
        isErr = 1;
      }
    }
    while (inPos < inLen)
    {
      a4 = input[inPos++] & 0xFF;
      if (isbase64 (a4))
      {
        break;
      }
      else if (a4 == '=')
      {
        isEndSeen = 1;
        break;
      }
      else if (a4 != '\r' && a4 != '\n' && a4 != ' ' && a4 != '\t')
      {
        isErr = 1;
      }
    }
    if (isbase64 (a1) && isbase64 (a2) && isbase64 (a3) && isbase64 (a4))
    {
      a1 = base64idx[a1] & 0xFF;
      a2 = base64idx[a2] & 0xFF;
      a3 = base64idx[a3] & 0xFF;
      a4 = base64idx[a4] & 0xFF;
      b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
      b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
      b3 = ((a3 << 6) & 0xC0) | (a4 & 0x3F);
      outStream.putch (b1);
      outStream.putch (b2);
      outStream.putch (b3);
    }
    else if (isbase64 (a1) && isbase64 (a2) && isbase64 (a3) && a4 == '=')
    {
      a1 = base64idx[a1] & 0xFF;
      a2 = base64idx[a2] & 0xFF;
      a3 = base64idx[a3] & 0xFF;
      b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
      b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
      outStream.putch (b1);
      outStream.putch (b2);
      break;
    }
    else if (isbase64 (a1) && isbase64 (a2) && a3 == '=' && a4 == '=')
    {
      a1 = base64idx[a1] & 0xFF;
      a2 = base64idx[a2] & 0xFF;
      b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
      outStream.putch (b1);
      break;
    }
    else
    {
      break;
    }
    if (isEndSeen)
    {
      break;
    }
  }                             /* end while loop */

  QCString deb (output.data (), output.count () + 1);

//  qDebug("rfcDecoder::decodeBase64 - give %d '%s'",output.count(),deb.data());

  return output;
}

const QCString
rfcDecoder::decodeQuotedPrintable (const QCString & _str)
{
  QByteArray pass, get;
  QCString retVal;

  pass.setRawData (_str.data (), _str.length ());
  get = decodeQuotedPrintable (pass);
  pass.resetRawData (_str.data (), _str.length ());
  retVal = QCString (get.data (), get.count () + 1);
  return retVal;
};

const QCString
rfcDecoder::encodeQuotedPrintable (const QCString & _str)
{
  QByteArray pass, get;
  QCString retVal;

  pass.setRawData (_str.data (), _str.length ());
  get = encodeQuotedPrintable (pass);
  pass.resetRawData (_str.data (), _str.length ());
  retVal = QCString (get.data (), get.count () + 1);
  return retVal;
};


const QByteArray
rfcDecoder::encodeQuotedPrintable (const QByteArray & input)
{
  QByteArray output;
  QBuffer outStream (output);
  outStream.open (IO_WriteOnly);

  size_t inPos, lineLen;
  int ch;
  ulong aInLen = input.count ();
  inPos = 0;
  lineLen = 0;

  while (inPos < aInLen)
  {
    ch = input[inPos++] & 0xFF;
    /* '.' at beginning of line (confuses some SMTPs) */
    if (lineLen == 0 && ch == '.')
    {
      outStream.putch ('=');
      outStream.putch (hextab[(ch >> 4) & 0x0F]);
      outStream.putch (hextab[ch & 0x0F]);
      lineLen += 3;
    }
    /* "From " at beginning of line (gets mangled in mbox folders) */
    else if (lineLen == 0 && inPos + 3 < aInLen && ch == 'F'
             && input[inPos] == 'r' && input[inPos + 1] == 'o'
             && input[inPos + 2] == 'm' && input[inPos + 3] == ' ')
    {
      outStream.putch ('=');
      outStream.putch (hextab[(ch >> 4) & 0x0F]);
      outStream.putch (hextab[ch & 0x0F]);
      lineLen += 3;
    }
    /* Normal printable char */
    else if ((62 <= ch && ch <= 126) || (33 <= ch && ch <= 60))
    {
      outStream.putch ((char) ch);
      ++lineLen;
    }
    /* Space */
    else if (ch == ' ')
    {
      /* Space at end of line or end of input must be encoded */
      if (inPos >= aInLen       /* End of input? */
          || (inPos < aInLen - 1  /* End of line? */
              && input[inPos] == '\r' && input[inPos + 1] == '\n'))
      {

        outStream.putch ('=');
        outStream.putch ('2');
        outStream.putch ('0');
        lineLen += 3;
      }
      else
      {
        outStream.putch (' ');
        ++lineLen;
      }
    }
    /* Hard line break */
    else if (inPos < aInLen && ch == '\r' && input[inPos] == '\n')
    {
      ++inPos;
      outStream.putch ('\r');
      outStream.putch ('\n');
      lineLen = 0;
    }
    /* Non-printable char */
    else if (ch & 0x80          /* 8-bit char */
             || !(ch & 0xE0)    /* control char */
             || ch == 0x7F      /* DEL */
             || ch == '=')
    {                           /* special case */
      outStream.putch ('=');
      outStream.putch (hextab[(ch >> 4) & 0x0F]);
      outStream.putch (hextab[ch & 0x0F]);
      lineLen += 3;
    }
    /* Soft line break */
    if (lineLen >= MAXLINE - 3 && !(inPos < aInLen - 1 &&
                                    input[inPos] == '\r'
                                    && input[inPos + 1] == '\n'))
    {

      outStream.putch ('=');
      outStream.putch ('\r');
      outStream.putch ('\n');
      lineLen = 0;
    }
  }
  return 0;
}


const QByteArray
rfcDecoder::decodeQuotedPrintable (const QByteArray & input)
{
  QByteArray output;
  QBuffer outStream (output);
  outStream.open (IO_WriteOnly);

  size_t i, inPos, aInLen, lineLen, nextLineStart, numChars, charsEnd;
  int isEolFound, softLineBrk, isError;
  int ch, c1, c2;

  isError = 0;
  inPos = 0;
  aInLen = input.count ();
  for (i = 0; i < aInLen; ++i)
  {
    if (input[i] == 0)
    {
      aInLen = i;
      break;
    }
  }
  if (aInLen == 0)
  {
    return input;
  }
  while (inPos < aInLen)
  {
    /* Get line */
//    qDebug("-> %d , %d",inPos,aInLen);
    lineLen = 0;
    isEolFound = 0;
    while (!isEolFound && lineLen < aInLen - inPos)
    {
      ch = input[inPos + lineLen];
      ++lineLen;
      if (ch == '\n')
      {
        isEolFound = 1;
      }
    }

    nextLineStart = inPos + lineLen;

//    qDebug("-> lineLen %d , %d",lineLen,nextLineStart);

    numChars = lineLen;
    /* Remove white space from end of line */
    while (numChars > 0)
    {
      ch = input[inPos + numChars - 1] & 0x7F;
      if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t')
      {
        break;
      }
      --numChars;
    }
    charsEnd = inPos + numChars;
    /* Decode line */
    softLineBrk = 0;
    while (inPos < charsEnd)
    {
//      qDebug("+> %d , %d",inPos,charsEnd);
      ch = input[inPos++] & 0x7F;
      if (ch != '=')
      {
        /* Normal printable char */
        outStream.putch ((char) ch);
      }
      else                      /* if (ch == '=') */
      {
        /* Soft line break */
        if (inPos >= charsEnd)
        {
          softLineBrk = 1;
          break;
        }
        /* Non-printable char */
        else if (inPos < charsEnd - 1)
        {
          c1 = input[inPos++] & 0x7F;
          if ('0' <= c1 && c1 <= '9')
            c1 -= '0';
          else if ('A' <= c1 && c1 <= 'F')
            c1 = c1 - 'A' + 10;
          else if ('a' <= c1 && c1 <= 'f')
            c1 = c1 - 'a' + 10;
          else
            isError = 1;
          c2 = input[inPos++] & 0x7F;
          if ('0' <= c2 && c2 <= '9')
            c2 -= '0';
          else if ('A' <= c2 && c2 <= 'F')
            c2 = c2 - 'A' + 10;
          else if ('a' <= c2 && c2 <= 'f')
            c2 = c2 - 'a' + 10;
          else
            isError = 1;
          outStream.putch ((char) ((c1 << 4) + c2));
        }
        else                    /* if (inPos == charsEnd-1) */
        {
          isError = 1;
        }
      }
    }
    if (isEolFound && !softLineBrk)
    {
      outStream.putch ('\r');
      outStream.putch ('\n');
    }
    inPos = nextLineStart;
  }
  return output;
}

// take from the c-client imap toolkit

/* Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 November 1989
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/*
 * RFC 2104 HMAC hashing
 * Accepts: text to hash
 *	    text length
 *	    key
 *	    key length
 * Returns: hash as text, always
 */

const QCString
rfcDecoder::encodeRFC2104 (const QCString & text, const QCString & key)
{
  int i, j;
  static char hshbuf[2 * MD5DIGLEN + 1];
  char *s;
  MD5CONTEXT ctx;
  char *hex = (char *) "0123456789abcdef";
  ulong keyLen = key.length ();
  unsigned char *keyPtr = (unsigned char *) key.data ();
  unsigned char digest[MD5DIGLEN], k_ipad[MD5BLKLEN + 1],
    k_opad[MD5BLKLEN + 1];
  if (key.length () > MD5BLKLEN)
  {                             /* key longer than pad length? */
    md5_init (&ctx);            /* yes, set key as MD5(key) */
    md5_update (&ctx, keyPtr, keyLen);
    md5_final (digest, &ctx);
    keyPtr = (unsigned char *) digest;
    keyLen = MD5DIGLEN;
  }
  memcpy (k_ipad, keyPtr, keyLen);  /* store key in pads */
  memset (k_ipad + keyLen, 0, (MD5BLKLEN + 1) - keyLen);
  memcpy (k_opad, k_ipad, MD5BLKLEN + 1);
  /* XOR key with ipad and opad values */
  for (i = 0; i < MD5BLKLEN; i++)
  {                             /* for each byte of pad */
    k_ipad[i] ^= 0x36;          /* XOR key with ipad */
    k_opad[i] ^= 0x5c;          /*  and opad values */
  }
  md5_init (&ctx);              /* inner MD5: hash ipad and text */
  md5_update (&ctx, k_ipad, MD5BLKLEN);
  md5_update (&ctx, (unsigned char *) text.data (), text.length ());
  md5_final (digest, &ctx);
  md5_init (&ctx);              /* outer MD5: hash opad and inner results */
  md5_update (&ctx, k_opad, MD5BLKLEN);
  md5_update (&ctx, digest, MD5DIGLEN);
  md5_final (digest, &ctx);
  /* convert to printable hex */
  for (i = 0, s = hshbuf; i < MD5DIGLEN; i++)
  {
    *s++ = hex[(j = digest[i]) >> 4];
    *s++ = hex[j & 0xf];
  }
  *s = '\0';                    /* tie off hash text */
  return QCString (hshbuf);
}
