#ifndef _BASE64_H
#define _BASE64_H "$Id$"

QString base64_decode_string(const char *string, unsigned int len);
QString base64_encode_string( const char *buf, unsigned int len );
QString create_auth_plain(const char *username, const char *pass);

#endif
