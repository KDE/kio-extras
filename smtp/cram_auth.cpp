// $Id$

// This file is roughly based on example code provided by the IETF for the
// first draft for the HMAC (keyed MD5) spec.
// All the QString bits are of course my fault. - alex

#include <qstring.h>

#include "base64.h"
#include "local_md5.h"

QString generate_cram_auth(const char *user, const char *pass, const char *shared)
{
	char i_key[65], o_key[65];
	int i;
	MD5 *imd5, *md5;
	QString ret, bshared;

	bshared=base64_decode_string(shared, strlen(shared));

	memset(i_key, 0, 65);
	memset(o_key, 0, 65);
	if (strlen(pass) > 64) {
		MD5 *kmd5 = new MD5;
		kmd5->update((unsigned char *)pass, strlen(pass));
		kmd5->finalize();
		memcpy(i_key, kmd5->raw_digest(), 16);
		memcpy(o_key, kmd5->raw_digest(), 16);
		delete kmd5;
	} else {
		memcpy(i_key, pass, strlen(pass));
		memcpy(o_key, pass, strlen(pass));
	}

	for (i=0; i < 64; i++) {
		i_key[i] ^= 0x36;
		o_key[i] ^= 0x5c;
	}

	imd5=new MD5;
	imd5->update((unsigned char *)i_key, 64);
	imd5->update((unsigned char *)bshared.latin1(), strlen(bshared.latin1()));
	imd5->finalize();

	md5= new MD5;
	md5->update((unsigned char *)o_key, 64);
	md5->update(imd5->raw_digest(), 16);
	md5->finalize();
	ret=QString(user);
	ret+=QString(" ")+QString(md5->hex_digest());

	delete imd5;
	delete md5;
	return base64_encode_string(ret.latin1(), ret.length());
}
