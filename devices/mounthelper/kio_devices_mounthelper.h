#ifndef KIO_DEVICES_MOUNTHELPER_H
#define KIO_DEVICES_MOUNTHELPER_H

#include <kapplication.h>

class KIODevicesMountHelperApp:public KApplication
{
        Q_OBJECT
public:
        KIODevicesMountHelperApp();
protected slots:
	void finished();
	void error();
};

#endif
