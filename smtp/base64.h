#ifndef _BASE64_H
#define _BASE64_H "$Id$"

	char *base64_encode_auth_line(const char *u, const char *p);
	char *base64_encode_string(const char *s, unsigned int len);

#endif
