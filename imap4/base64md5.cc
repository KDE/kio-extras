
// Modified by Stefan Taferner <taferner@kde.org> for KMail2
//

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "base64md5.h"
#include "md5.h"

static int encode_base64(const char* aIn, size_t aInLen, char* aOut,
			 size_t aOutSize, size_t* aOutLen);
static int decode_base64(const char* aIn, size_t aInLen, char* aOut,
			 size_t aOutSize, size_t* aOutLen);

const QString encodeBase64(const QString aStr, int* aLenPtr)
{
  size_t srcLen, destLen, destSize;
  char* srcBuf;
  char destBuf[1024];
  int result;

  // Estimate required destination buffer size
  srcLen = aStr.length();
//  srcBuf = aStr.data();
  srcBuf = aStr.latin1();
  destSize = (srcLen+2)/3*4;
  destSize += destSize/72 + 2;
  destSize += 64;  // a little extra room

  // Allocate destination buffer
//  QString destStr(destSize);
//  QString destStr(new QChar[destSize], destSize);
//  destBuf = (char*)destStr.data();

  // Encode source to destination
  result = encode_base64(srcBuf, srcLen, destBuf, destSize, &destLen);
//  debug(QString("<encode> %1 -> %2").arg(srcBuf).arg(destBuf));
  if (result) destLen = 0;

//  QString sResult = destStr.left(destLen);
  QString sResult(destBuf);
  if (aLenPtr) *aLenPtr = destLen;
//  debug(QString("<encode> return %1").arg(sResult));
  return sResult;
}


//-----------------------------------------------------------------------------
const QString decodeBase64(const QString aStr, int* aLenPtr)
{
  size_t srcLen, destLen, destSize;
  QString result;
  int rc;

  // Set destination buffer size same as source buffer size
  srcLen = aStr.length();
  destSize = srcLen;

  // Allocate destination buffer
  char destStr[destSize*10];
  destSize = destSize * 10;

  // Encode source to destination
  rc = decode_base64(aStr.ascii(), srcLen, destStr, destSize, &destLen);
  if (rc) destLen = 0;

//  result.resize(destLen+1);
//  memcpy(result.data(), destStr, destLen);
  result = destStr;
  result[destLen] = '\0';

//  debug(QString("<decode> %1 -> %2 len=%3").arg(aStr).arg(destStr).arg(destLen));
  if (aLenPtr) *aLenPtr = destLen;
  return result;
}

const QString encodeMD5(const QString & src) {
  MD5_CTX context;
  unsigned char md5text[16];
  QString result;

  MD5Init(&context);
  MD5Update(&context, reinterpret_cast<const unsigned char *>(src.latin1()), src.length());
  MD5Final(md5text, &context);
  QString h = "0123456789abcdef";
  for(int i=0; i<16; i++) {
    int c = md5text[i];
    result += h[(c >> 4) & 0xF];
    result += h[c & 0xF];
  }
  return QString(result);
}

//=============================================================================
// Code below is:
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

#define MAXLINE  76

static char base64tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz0123456789+/";

static char base64idx[128] = {
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377',    62,'\377','\377','\377',    63,
        52,    53,    54,    55,    56,    57,    58,    59,
        60,    61,'\377','\377','\377','\377','\377','\377',
    '\377',     0,     1,     2,     3,     4,     5,     6,
         7,     8,     9,    10,    11,    12,    13,    14,
        15,    16,    17,    18,    19,    20,    21,    22,
        23,    24,    25,'\377','\377','\377','\377','\377',
    '\377',    26,    27,    28,    29,    30,    31,    32,
        33,    34,    35,    36,    37,    38,    39,    40,
        41,    42,    43,    44,    45,    46,    47,    48,
        49,    50,    51,'\377','\377','\377','\377','\377'
};

static char hextab[] = "0123456789ABCDEF";

#ifdef __cplusplus
inline int isbase64(int a) {
    return ('A' <= a && a <= 'Z')
        || ('a' <= a && a <= 'z')
        || ('0' <= a && a <= '9')
        || a == '+' || a == '/';
}
#else
#define isbase64(a) (  ('A' <= (a) && (a) <= 'Z') \
                    || ('a' <= (a) && (a) <= 'z') \
                    || ('0' <= (a) && (a) <= '9') \
                    ||  (a) == '+' || (a) == '/'  )
#endif


static int encode_base64(const char* aIn, size_t aInLen, char* aOut,
    size_t aOutSize, size_t* aOutLen)
{
    if (!aIn || !aOut || !aOutLen)
        return -1;
    size_t inLen = aInLen;
    char* out = aOut;
    size_t outSize = (inLen+2)/3*4;     /* 3:4 conversion ratio */
    outSize += outSize/MAXLINE + 2;  /* Space for newlines and NUL */
    if (aOutSize < outSize)
        return -1;
    size_t inPos  = 0;
    size_t outPos = 0;
    int c1, c2, c3;
    int lineLen = 0;
    /* Get three characters at a time and encode them. */
    for (size_t i=0; i < inLen/3; ++i) {
        c1 = aIn[inPos++] & 0xFF;
        c2 = aIn[inPos++] & 0xFF;
        c3 = aIn[inPos++] & 0xFF;
        out[outPos++] = base64tab[(c1 & 0xFC) >> 2];
        out[outPos++] = base64tab[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)];
        out[outPos++] = base64tab[((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6)];
        out[outPos++] = base64tab[c3 & 0x3F];
		lineLen += 4;
        if (lineLen >= MAXLINE-3) {
			char* cp = "\n";
            out[outPos++] = *cp++;
			if (*cp) {
				out[outPos++] = *cp;
			}
			lineLen = 0;
        }
    }
    /* Encode the remaining one or two characters. */
	char* cp;
    switch (inLen % 3) {
    case 0:
		cp = "\n";
        out[outPos++] = *cp++;
		if (*cp) {
			out[outPos++] = *cp;
		}
        break;
    case 1:
        c1 = aIn[inPos] & 0xFF;
        out[outPos++] = base64tab[(c1 & 0xFC) >> 2];
        out[outPos++] = base64tab[((c1 & 0x03) << 4)];
        out[outPos++] = '=';
        out[outPos++] = '=';
		cp = "\n";
        out[outPos++] = *cp++;
		if (*cp) {
			out[outPos++] = *cp;
		}
        break;
    case 2:
        c1 = aIn[inPos++] & 0xFF;
        c2 = aIn[inPos] & 0xFF;
        out[outPos++] = base64tab[(c1 & 0xFC) >> 2];
        out[outPos++] = base64tab[((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4)];
        out[outPos++] = base64tab[((c2 & 0x0F) << 2)];
        out[outPos++] = '=';
		cp = "\n";
        out[outPos++] = *cp++;
		if (*cp) {
			out[outPos++] = *cp;
		}
        break;
    }
    out[outPos] = 0;
    *aOutLen = outPos;
    return 0;
}


static int decode_base64(const char* aIn, size_t aInLen, char* aOut,
    size_t aOutSize, size_t* aOutLen)
{
    if (!aIn || !aOut || !aOutLen)
        return -1;
    size_t inLen = aInLen;
    char* out = aOut;
    size_t outSize = (inLen/4+1)*3+1;
    if (aOutSize < outSize)
        return -1;
    /* Get four input chars at a time and decode them. Ignore white space
     * chars (CR, LF, SP, HT). If '=' is encountered, terminate input. If
     * a char other than white space, base64 char, or '=' is encountered,
     * flag an input error, but otherwise ignore the char.
     */
    int isErr = 0;
    int isEndSeen = 0;
    int b1, b2, b3;
    int a1, a2, a3, a4;
    size_t inPos = 0;
    size_t outPos = 0;
    while (inPos < inLen) {
        a1 = a2 = a3 = a4 = 0;
        while (inPos < inLen) {
            a1 = aIn[inPos++] & 0xFF;
            if (isbase64(a1)) {
                break;
            }
            else if (a1 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a1 != '\r' && a1 != '\n' && a1 != ' ' && a1 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a2 = aIn[inPos++] & 0xFF;
            if (isbase64(a2)) {
                break;
            }
            else if (a2 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a2 != '\r' && a2 != '\n' && a2 != ' ' && a2 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a3 = aIn[inPos++] & 0xFF;
            if (isbase64(a3)) {
                break;
            }
            else if (a3 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a3 != '\r' && a3 != '\n' && a3 != ' ' && a3 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a4 = aIn[inPos++] & 0xFF;
            if (isbase64(a4)) {
                break;
            }
            else if (a4 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a4 != '\r' && a4 != '\n' && a4 != ' ' && a4 != '\t') {
                isErr = 1;
            }
        }
        if (isbase64(a1) && isbase64(a2) && isbase64(a3) && isbase64(a4)) {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            a4 = base64idx[a4] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            b3 = ((a3 << 6) & 0xC0) | ( a4       & 0x3F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            out[outPos++] = char(b3);
        }
        else if (isbase64(a1) && isbase64(a2) && isbase64(a3) && a4 == '=') {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            break;
        }
        else if (isbase64(a1) && isbase64(a2) && a3 == '=' && a4 == '=') {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            out[outPos++] = char(b1);
            break;
        }
        else {
            break;
        }
        if (isEndSeen) {
            break;
        }
    } /* end while loop */
    *aOutLen = outPos;
    return (isErr) ? -1 : 0;
}

