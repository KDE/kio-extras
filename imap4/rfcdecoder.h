#ifndef RFCDECODER_H
#define RFCDECODER_H

#include <qstring.h>

// namespace for our rfc en/decoders
class rfcDecoder {

  public:
	
	static const QString fromIMAP(const QString &src);
	static const QString toIMAP(const QString &inSrc);

	// decoder for RFC2047 and RFC1522
	static const QString decodeRFC2047String(const QString& _str,QString &charset,QString &language);
	static const QString decodeRFC2047String(const QString& _str,QString &charset);
	static const QString decodeRFC2047String(const QString& _str);

	// encoder for RFC2047 and RFC1522
	static const QString encodeRFC2047String(const QString& _str,QString &charset,QString &language);
	static const QString encodeRFC2047String(const QString& _str,QString &charset);
	static const QString encodeRFC2047String(const QString& _str);

	static const QString encodeRFC2231String(const QString& _str);
	static const QString decodeRFC2231String(const QString& _str);

	//these are ByteArray because there is no interpretation of
	//a charset or whatsoever here
	static const QByteArray decodeBase64(const QByteArray &_str);
	static const QByteArray encodeBase64(const QByteArray& _str);

	//convenience functions beware! (of 0x00 that is)
	static const QCString decodeBase64(const QCString &_str);
	static const QCString encodeBase64(const QCString &_str);
	
	static const QByteArray decodeQuotedPrintable(const QByteArray& _str);
	static const QByteArray encodeQuotedPrintable(const QByteArray& _str);

	//convenience functions beware! (of 0x00 that is)
	static const QCString decodeQuotedPrintable(const QCString &_str);
	static const QCString encodeQuotedPrintable(const QCString &_str);
	

	//for the authenticator cram-md5 (no decoder apparently)
	static const QCString encodeRFC2104(const QCString &text,const QCString &key);
};

#endif
