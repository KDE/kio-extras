#include "emailsettings.h"

#include <kconfig.h>

class KEMailSettingsPrivate {
public:
	KConfig *m_pConfig;
	QStringList profiles;
	QString m_sDefaultProfile, m_sCurrentProfile;
};

QString KEMailSettings::defaultProfileName()
{
	return p->m_sDefaultProfile.copy();
}

QString KEMailSettings::getSetting(KEMailSettings::Setting s)
{
	p->m_pConfig->setGroup(QString("PROFILE_")+p->m_sCurrentProfile);
	switch (s) {
		case ClientProgram: {
			return p->m_pConfig->readEntry("EmailClient");
			break;
		}
		case ClientTerminal: {
			return ((p->m_pConfig->readBoolEntry("TerminalClient")) ? QString("true") : QString("false") );
			break;
		}
		case RealName: {
			return p->m_pConfig->readEntry("FullName");
			break;
		}
		case EmailAddress: {
			return p->m_pConfig->readEntry("EmailAddress");
			break;
		}
		case ReplyToAddress: {
			return p->m_pConfig->readEntry("ReplyAddr");
			break;
		}
		case Organization: {
			return p->m_pConfig->readEntry("Organization");
			break;
		}
		case OutServer: {
			return p->m_pConfig->readEntry("Outgoing");
			break;
		}
		case InServer: {
			return p->m_pConfig->readEntry("Incoming");
			break;
		}
		case InServerLogin: {
			return p->m_pConfig->readEntry("UserName");
			break;
		}
		case InServerPass: {
			return p->m_pConfig->readEntry("Password");
			break;
		}
		case InServerType: {
			return p->m_pConfig->readEntry("ServerType");
			break;
		}
	};
	return QString::null;
}
void KEMailSettings::setSetting(KEMailSettings::Setting s, const QString  &v)
{
	p->m_pConfig->setGroup(QString("PROFILE_")+p->m_sCurrentProfile);
	switch (s) {
		case ClientProgram: {
			p->m_pConfig->writeEntry("EmailClient", v);
			break;
		}
		case ClientTerminal: {
			p->m_pConfig->writeEntry("TerminalClient", (v == "true") ? true : false );
			break;
		}
		case RealName: {
			p->m_pConfig->writeEntry("FullName", v);
			break;
		}
		case EmailAddress: {
			p->m_pConfig->writeEntry("EmailAddress", v);
			break;
		}
		case ReplyToAddress: {
			p->m_pConfig->writeEntry("ReplyAddr", v);
			break;
		}
		case Organization: {
			p->m_pConfig->writeEntry("Organization", v);
			break;
		}
		case OutServer: {
			p->m_pConfig->writeEntry("Outgoing", v);
			break;
		}
		case InServer: {
			p->m_pConfig->writeEntry("Incoming", v);
			break;
		}
		case InServerLogin: {
			p->m_pConfig->writeEntry("UserName", v);
			break;
		}
		case InServerPass: {
			p->m_pConfig->writeEntry("Password", v);
			break;
		}
		case InServerType: {
			p->m_pConfig->writeEntry("ServerType", v);
			break;
		}
	};
	p->m_pConfig->sync();
}

void KEMailSettings::setDefault(const QString &s)
{
	p->m_pConfig->setGroup("Defaults");
	p->m_pConfig->writeEntry("Profile", s);
	p->m_pConfig->sync();
}

void KEMailSettings::setProfile (const QString &s)
{
	p->m_sCurrentProfile=s.copy();
}

QString KEMailSettings::currentProfileName()
{
	return p->m_sCurrentProfile;
}

QStringList KEMailSettings::profiles()
{
	return p->profiles;
}

KEMailSettings::KEMailSettings()
{
	p = new KEMailSettingsPrivate();
	p->m_sCurrentProfile=QString::null;

	p->m_pConfig = new KConfig("emaildefaults");

	QStringList groups = p->m_pConfig->groupList();
	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it) {
		if ( (*it).left(8) == "PROFILE_" )
			p->profiles+= (*it).mid(8, (*it).length());
	}

	p->m_pConfig->setGroup("Defaults");
	p->m_sDefaultProfile=p->m_pConfig->readEntry("Profile");
	if (p->m_sDefaultProfile != QString::null) {
		if (!p->m_pConfig->hasGroup(QString("PROFILE_")+p->m_sDefaultProfile))
			p->m_sDefaultProfile=QString::null;
	}
}

KEMailSettings::~KEMailSettings()
{
}
