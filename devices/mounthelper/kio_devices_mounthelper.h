#ifndef KIO_DEVICES_MOUNTHELPER_H
#define KIO_DEVICES_MOUNTHELPER_H

#include <kapplication.h>
#include <qstring.h>
#include <kio/job.h>

class KIODevicesMountHelperApp:public KApplication
{
        Q_OBJECT
public:
        KIODevicesMountHelperApp();
private:
	QStringList deviceInfo(QString name);
	QString errorStr;
protected slots:
	void slotResult(KIO::Job* job);
	void finished();
	void error();
};

#endif
