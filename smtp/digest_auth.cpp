// $Id$

#if 0
  the #rule
	A construct "#" is defined, similar to "*", for defining lists of
	elements. The full form is "&lt;n>#&lt;m>element" indicating at
	least <n> and at most <m> elements, each separated by one or more
	commas (",") and OPTIONAL linear white space (LWS). This makes the
	usual form of lists very easy; a rule such as
		( *LWS element *( *LWS "," *LWS element ))
	can be shown as
		1#element
#endif

#include <sys/types.h>
#include <sys/uio.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "local_md5.h"

#include <iostream.h>

#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>

#include <kdebug.h>
#include <kurl.h>

#include "base64.h"
#include "digest_auth.h"

DigestAuth::DigestAuth(const KURL &request, const char *l, bool _isBASE64)
{
	UseChallenge(request, l, _isBASE64);
}

void DigestAuth::UseChallenge(const KURL &request, const char *l, bool _isBASE64)
{
	Reset();
	isBASE64=_isBASE64;
	request_url=KURL(request);
	if (!isBASE64)
		UseAuthString(l);
	else
		UseAuthString(base64_decode_string(l, strlen(l)));
}

void GetA1(const QString &user, const QString &realm, const QString &pass, const QString &nonce, const QString &cnonce, MD5 &A1)
{
	MD5 *UserHash;

	UserHash = new MD5;
	UserHash->update((unsigned char *)user.latin1(), strlen(user.latin1()));
	UserHash->update((unsigned char *)":", 1);
	UserHash->update((unsigned char *)realm.latin1(), strlen(realm.latin1()));
	UserHash->update((unsigned char *)":", 1);
	UserHash->update((unsigned char *)pass.latin1(), strlen(pass.latin1()));
	UserHash->finalize();

	A1.update(UserHash->raw_digest(), 16);
	A1.update((unsigned char *)":", 1);
	A1.update((unsigned char *)nonce.latin1(), strlen(nonce.latin1()));
	A1.update((unsigned char *)":", 1);
	A1.update((unsigned char *)cnonce.latin1(), strlen(cnonce.latin1()));
	A1.finalize();
	delete UserHash;
}

void GetA2(const QString &qop, const QString &uri, MD5 &A2)
{
	A2.update((unsigned char *)"AUTHENTICATE", strlen("AUTHENTICATE"));
	A2.update((unsigned char *)":", 1);
	A2.update((unsigned char *)uri.latin1(), strlen(uri.latin1()));
	if ( (qop == "auth-int") ||  (qop == "auth-conf") ) {
		char buf[34];
		sprintf(buf, ":%.32d", 0);
		A2.update((unsigned char *)buf, strlen(buf));
	}
	A2.finalize();
}

bool DigestAuth::GenerateResponse(QString &s)
{
	char nc[9];
	QString uri, response;
	MD5 A1, A2, FINAL;

	GetA1(request_url.user(), keys["realm"], request_url.pass(),keys["nonce"], QString(cnonce), A1);

	uri=request_url.protocol()+QString("/")+request_url.host()+QString("/")+request_url.host();
	GetA2("auth", uri, A2);

        FINAL.update((unsigned char *)A1.hex_digest(), 32);
        FINAL.update((unsigned char *)":", 1);
        FINAL.update((unsigned char *)keys["nonce"].latin1(), strlen(keys["nonce"].latin1()));
        FINAL.update((unsigned char *)":", 1);
	sprintf(nc, "%.8d", ++nonce_count);
        FINAL.update((unsigned char *)nc, strlen(nc));
        FINAL.update((unsigned char *)":", 1);
        FINAL.update((unsigned char *)cnonce, strlen(cnonce));
        FINAL.update((unsigned char *)":", 1);
        FINAL.update((unsigned char *)"auth", strlen("auth"));
        FINAL.update((unsigned char *)":", 1);
        FINAL.update((unsigned char *)A2.hex_digest(), 32);
        FINAL.finalize();

	response=QString("charset=utf-8,");
	response+=QString("username=\"")+request_url.user()+QString("\",");
	response+=QString("realm=\"")+keys["realm"]+QString("\",");
	response+=QString("nonce=\"")+keys["nonce"]+QString("\",");
	response+=QString("nc=")+nc+QString(",");
	response+=QString("cnonce=\"")+QString(cnonce)+QString("\",");
	response+=QString("digest-uri=\"")+uri+QString("\",");
	response+=QString("response=")+QString(FINAL.hex_digest())+QString(",");
	response+=QString("qop=auth");
	if (isBASE64)
		s=base64_encode_string(response.latin1(), strlen(response.latin1()));
	else
		s=response.copy();
	return true;
}

void DigestAuth::Reset()
{
	memset(&cnonce, 0, 128);
	QFile f("/dev/random");
	if (f.open(IO_ReadOnly)) {
		::read(f.handle(), cnonce, 127); // Read up to 127 bits of randomness
		f.close();
	} else {
		sprintf(cnonce, "%s", "AHHYOUNEEDRANDOMINYOURLIFE");
	}
	keys.clear();
	nonce_count=0;
}

void DigestAuth::AddKey(const QString &k, const QString &v, QMap<QString, QString> &m)
{
	kdDebug() << "Adding " << k.latin1() << ":" << v.latin1() << endl;
	m.insert(k, v);
}

void DigestAuth::AddKey(const QString &line, QMap<QString, QString> &)
{
	bool open_quote=false;
	unsigned int i=0;
	QString authstr(line);
	while (authstr.length() >= i) {
		if (authstr.at(i) == '\"')
			if (open_quote) open_quote=false;
			else open_quote=true;
		else if (authstr.at(i) == '=') {
			if (!open_quote) {
				QString left, right;
			left=authstr.mid(0, i);
			right=authstr.mid(i+1, authstr.length());
			if (right.at(0) == '\"')
				right=right.mid(1, right.length());
			if (right.at(right.length()-1) == '\"' )
				right=right.mid(0, right.length()-1);
			AddKey(left, right, keys);	
			}
		}
		i++;
	}
}

void DigestAuth::UseAuthString(const QString &authstr)
{
	bool open_quote=false;
	unsigned int i=0, comma=0;
	kdDebug() << "Authstring is: " << authstr << endl;
	while (authstr.length() >= i) {
		if (authstr.at(i) == '\"')
			if (open_quote) open_quote=false;
			else open_quote=true;
		else if (authstr.at(i) == ',') {
			if (!open_quote) {
				AddKey(authstr.mid(comma, i-comma), keys);
				comma=i+1;
				if (authstr.find(",", i+1) == -1) {
					AddKey(authstr.mid(i+1, authstr.length()), keys);
					break;
				}
			}
		}
		i++;
	}
}

#if 0
int main()
{
const char *postfix="cmVhbG09InppcHB5LnBhY2JlbGwubmV0Iixub25jZT0idEhKbmRhNEZrU0pvaTAxOXFNcUFHcjg4MFpidnhtbkVwd1hxeGRRamR4VT0iLHFvcD0iYXV0aCxhdXRoLWludCIsY2hhcnNldD11dGYtOCxhbGdvcml0aG09bWQ1LXNlc3M=";

	KURL u("smtp://testuser:test@zippy.pacbell.net/");
	DigestAuth *d=new DigestAuth(u, postfix, true);
	kdDebug() << "Initial challenge is: " << postfix << endl << endl << endl;

	QString r;
	d->GenerateResponse(r);
	printf("it wants me to send %s\n\n",r.latin1());
	//d->GenerateResponse(r);
	//printf("it wants me to send %s\n",r.latin1());
	return 0;
}
#endif
