#ifndef _EMAILSETTINGS_H
#define _EMAILSETTINGS_H "$Id$"

#include <qstring.h>
#include <qstringlist.h>

class KEMailSettingsPrivate;

class KEMailSettings {
public:
	enum Setting {
		ClientProgram,
		ClientTerminal,
		RealName,
		EmailAddress,
		ReplyToAddress,
		Organization,
		OutServer,
		InServer,
		InServerLogin,
		InServerPass,
		InServerType
	};

	enum Extension {
		POP3,
		SMTP,
		OTHER
	};

	KEMailSettings();
	~KEMailSettings();
	QStringList profiles();

	// Retr what profile we're currently using
	QString currentProfileName();

	// Change the current profile
	void setProfile (const QString &);

	// Retr the name of the one that's currently default QString::null if none
	QString defaultProfileName();

	// New default..
	void setDefault(const QString &);

	// Get a "basic" setting, one that I've already thought of..
	QString getSetting(KEMailSettings::Setting s);
	void setSetting(KEMailSettings::Setting s, const QString &v);

	// Use this when trying to get at currently unimplemented settings
	// such as POP3 authentication methods, or mail specific TLS settings
	// or something I haven't already thought of
	QString getExtendedSetting(KEMailSettings::Extension e, const QString &s );
	void setExtendedSetting(KEMailSettings::Extension e, const QString &s, const QString &v );

protected:
	KEMailSettingsPrivate *p;
};

#endif
