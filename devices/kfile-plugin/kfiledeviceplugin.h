#ifndef __KFILE_DEVICE_H__
#define __KFILE_DEVICE_H__

#include <kfilemetainfo.h>
#include <kurl.h>

class KFileDevicePlugin : public KFilePlugin
{
	Q_OBJECT
public:
	KFileDevicePlugin(QObject *parent, const char *name,
	                  const QStringList &args);

	bool readInfo(KFileMetaInfo &info, uint what = KFileMetaInfo::Fastest);

private:
	void addMimeType(const char *mimeType);
	QString askMountPoint(KFileMetaInfo &info);

	unsigned long m_total;
	unsigned long m_used;
	unsigned long m_free;

private slots:
	void slotFoundMountPoint(const QString &mountPoint,
	                         unsigned long total, unsigned long used,
	                         unsigned long available);
	void slotDfDone();
};

#endif
