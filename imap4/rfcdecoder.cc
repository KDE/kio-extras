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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   Send comments and bug fixes to s.carstens@gmx.de
 *
 *********************************************************************/
#include "rfcdecoder.h"

#include <ctype.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

#include <qtextcodec.h>
#include <qbuffer.h>
#include <qregexp.h>
#include <kmdcodec.h>

// This part taken from rfc 2192 IMAP URL Scheme. C. Newman. September 1997.
// adapted to QT-Toolkit by Sven Carstens <s.carstens@gmx.de> 2000

static unsigned char base64chars[] =
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
QString rfcDecoder::fromIMAP (const QString & inSrc)
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
    base64[(int)base64chars[i]] = i;
  }

  /* loop until end of string */
  while (srcPtr < src.length ())
  {
    c = src[srcPtr++];
    /* deal with literal characters and &- */
    if (c != '&' || src[srcPtr] == '-')
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

/* replace " with \" and \ with \\ " and \ characters */
QString rfcDecoder::quoteIMAP(const QString &src)
{
  QString result;
  for (unsigned int i = 0; i < src.length(); i++)
  {
    if (src[i] == '"' || src[i] == '\\') result += '\\';
    result += src[i];
  }
  return result;
}

/* Convert Unicode path to modified UTF-7 IMAP mailbox
 */
QString rfcDecoder::toIMAP (const QString & inSrc)
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
          bitstogo = 0;
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
  return quoteIMAP(dst);
}

//-----------------------------------------------------------------------------
QString rfcDecoder::decodeQuoting(const QString &aStr)
{
  QString result;
  for (unsigned int i = 0; i < aStr.length(); i++)
  {
    if (aStr[i] == "\\") i++;
    result += aStr[i];
  }
  return result;
}

//-----------------------------------------------------------------------------
QTextCodec *
rfcDecoder::codecForName (const QString & _str)
{
  if (_str.isEmpty ())
    return NULL;
  return QTextCodec::codecForName (_str.lower ().
                                   replace ("windows", "cp").latin1 ());
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
  char *pos, *beg, *end, *mid = NULL;
  QCString str;
  char encoding = 0, ch;
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
//    kdDebug(7116) << "rfcDecoder::decodeRFC2047String - charset " << charset << " - language " << language << " - '" << pos << "'" << endl;
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
//    kdDebug(7116) << "rfcDecoder::decodeRFC2047String - before QP '" << str << "'" << endl;

        str = KCodecs::quotedPrintableDecode(str);
//    kdDebug(7116) << "rfcDecoder::decodeRFC2047String - after QP '" << str << "'" << endl;
      }
      else
      {
        // decode base64 text
        str = KCodecs::base64Decode(str);
      }
      *pos = ch;
      for (i = 0; i < (int) str.length (); i++)
        result += (char) (QChar) str[i];

      pos = end - 1;
    }
    else
    {
//    kdDebug(7116) << "rfcDecoder::decodeRFC2047String - invalid" << endl;
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
//    kdDebug(7116) << "Codec is " << aCodec->name() << endl;
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

  //kdDebug(7116) << "Charset: " << charset << " Language: " << language << endl;

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
