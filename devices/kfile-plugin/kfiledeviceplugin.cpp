#include "kfiledeviceplugin.h"
#include <kgenericfactory.h>
#include <dcopclient.h>
#include <qdatastream.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qeventloop.h>
#include <kdiskfreesp.h>
#include <kapplication.h>
#include <unistd.h>
#include <kdebug.h>

typedef KGenericFactory<KFileDevicePlugin> KFileDevicePluginFactory;
K_EXPORT_COMPONENT_FACTORY(kfile_device, KFileDevicePluginFactory("kfile_device"))

KFileDevicePlugin::KFileDevicePlugin(QObject *parent, const char *name,
		                     const QStringList& args)
	: KFilePlugin(parent, name, args)
{
	addMimeType( "kdedevice/cdrom_mounted" );
	addMimeType( "kdedevice/cdwriter_mounted" );
	addMimeType( "kdedevice/dvd_mounted" );
	addMimeType( "kdedevice/floppy5_mounted" );
	addMimeType( "kdedevice/floppy_mounted" );
	addMimeType( "kdedevice/hdd_mounted" );
	addMimeType( "kdedevice/removable_mounted" );
	addMimeType( "kdedevice/zip_mounted" );
}

bool KFileDevicePlugin::readInfo(KFileMetaInfo &info, uint /*what*/)
{
	QString mount_point = askMountPoint(info);
	
	KDiskFreeSp *df = new KDiskFreeSp();
	
	m_total = 0;
	m_used = 0;
	m_free = 0;
	connect(df, SIGNAL( done() ), this, SLOT( slotDfDone() ));
	connect(df, SIGNAL( foundMountPoint(const QString &,
	                                    unsigned long,
	                                    unsigned long,
			                    unsigned long) ),
	        this, SLOT( slotFoundMountPoint(const QString &,
		                                unsigned long,
	                                        unsigned long,
	                                        unsigned long)) );
	
	df->readDF(mount_point);
	
	qApp->eventLoop()->enterLoop();

	int percent = 0;
	int length = 0;
	
	if (m_total != 0)
	{
		percent = 100 * m_used / m_total;
		length = 150 * m_used / m_total;	
	}
	
	KFileMetaInfoGroup group = appendGroup(info, "deviceInfo");
	
	appendItem(group, "free", (long long unsigned)m_free);
	appendItem(group, "used", (long long unsigned)m_used);
	appendItem(group, "total", (long long unsigned)m_total);

	group = appendGroup(info, "deviceSummary");
	
	appendItem(group, "percent", QString("%1\%").arg(percent));
	
	QPixmap bar(150, 20);
	QPainter p(&bar);
		
	p.fillRect(0, 0, length, 20, Qt::red);
	p.fillRect(length, 0, 150-length, 20, Qt::green);
	
	QColorGroup cg = QApplication::palette().active();
	
	QApplication::style().drawPrimitive(QStyle::PE_Panel, &p,
	                                    QRect(0, 0, 150, 20), cg,
	                                    QStyle::Style_Sunken);
		
	appendItem( group, "thumbnail", bar );
	
	return true;
}

void KFileDevicePlugin::slotFoundMountPoint(const QString &/*mountPoint*/,
                                            unsigned long total, unsigned long used,
                                            unsigned long free)
{
	m_free = free;
	m_used = used;
	m_total = total;
}

void KFileDevicePlugin::slotDfDone()
{
	qApp->eventLoop()->exitLoop();
}

QString KFileDevicePlugin::askMountPoint(KFileMetaInfo &info)
{
	QByteArray data;
	QByteArray param;
	QCString retType;
	QStringList retVal;
	QDataStream streamout(param,IO_WriteOnly);
	streamout<<info.url().fileName();

	DCOPClient *client = kapp->dcopClient();

	if(client->call("kded", "mountwatcher", "basicDeviceInfo(QString)",
	                param,retType,data,false )
	  )
	{
		QDataStream streamin(data,IO_ReadOnly);
		streamin>>retVal;

		return KURL(retVal[3]).path();
	}

	return QString::null;
}

void KFileDevicePlugin::addMimeType(const char *mimeType)
{
	KFileMimeTypeInfo *info = addMimeTypeInfo( mimeType );

	KFileMimeTypeInfo::GroupInfo *group
		= addGroupInfo(info, "deviceInfo", i18n("Device Information"));

	KFileMimeTypeInfo::ItemInfo *item
		= addItemInfo(group, "free", i18n("Free"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);

	item = addItemInfo(group, "used", i18n("Used"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);

	item = addItemInfo(group, "total", i18n("Total"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);


	group = addGroupInfo(info, "deviceSummary", i18n("Device Summary"));
	
	item = addItemInfo(group, "percent", i18n("Usage"), QVariant::String);
	
	item = addItemInfo( group, "thumbnail", i18n("Bar graph"), QVariant::Image );
	setHint( item, KFileMimeTypeInfo::Thumbnail );
}

#include "kfiledeviceplugin.moc"
