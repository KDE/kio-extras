// $Id$

#ifndef DIGEST_AUTH_H
#define DIGEST_AUTH_H "$Id$"

#include <qmap.h>

#include <kurl.h>

class DigestAuth
{
public:
	DigestAuth() {;}
	DigestAuth(const KURL &request, const char *initial_challenge, bool isBASE64=false);
	~DigestAuth();

	void Reset();
	void UseChallenge(const KURL &request, const char *initial_challenge, bool isBASE64=false);
	bool GenerateResponse(QString &s);
protected:

	bool isBASE64;
	unsigned int nonce_count;
	char cnonce[128];
	KURL request_url;

	void UseAuthString(const QString &line);
	void AddKey(const QString &line, QMap<QString, QString> &);
	void AddKey(const QString &k, const QString &v, QMap<QString, QString> &m);
	QMap<QString,QString> keys;

};

#endif
