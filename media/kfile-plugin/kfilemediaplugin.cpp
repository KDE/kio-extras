/* This file is part of the KDE project
   Copyright (C) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfilemediaplugin.h"

#include <kgenericfactory.h>
#include <kdiskfreesp.h>

#include <QtDBus/QtDBus>

#include <QPixmap>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QEventLoop>

typedef KGenericFactory<KFileMediaPlugin> KFileMediaPluginFactory;
K_EXPORT_COMPONENT_FACTORY(kfile_media, KFileMediaPluginFactory("kio_media"))

KFileMediaPlugin::KFileMediaPlugin(QObject *parent, const QStringList& args)
	: KFilePlugin(parent, args)
{
	addMimeType( "media/audiocd" );
	addMimeType( "media/hdd_mounted" );
	addMimeType( "media/blankcd" );
	addMimeType( "media/hdd_unmounted" );
	addMimeType( "media/blankdvd" );
	addMimeType( "media/cdrom_mounted" );
	addMimeType( "media/cdrom_unmounted" );
	addMimeType( "media/cdwriter_mounted" );
	addMimeType( "media/nfs_mounted" );
	addMimeType( "media/cdwriter_unmounted" );
	addMimeType( "media/nfs_unmounted" );
	addMimeType( "media/removable_mounted" );
	addMimeType( "media/dvd_mounted" );
	addMimeType( "media/removable_unmounted" );
	addMimeType( "media/dvd_unmounted" );
	addMimeType( "media/smb_mounted" );
	addMimeType( "media/dvdvideo" );
	addMimeType( "media/smb_unmounted" );
	addMimeType( "media/floppy5_mounted" );
	addMimeType( "media/svcd" );
	addMimeType( "media/floppy5_unmounted" );
	addMimeType( "media/vcd" );
	addMimeType( "media/floppy_mounted" );
	addMimeType( "media/zip_mounted" );
	addMimeType( "media/floppy_unmounted" );
	addMimeType( "media/zip_unmounted" );
	addMimeType( "media/gphoto2camera" );
}

bool KFileMediaPlugin::readInfo(KFileMetaInfo &info, uint /*what*/)
{
	const Medium medium = askMedium(info);

	if (medium.id().isNull()) return false;

	QString mount_point = medium.mountPoint();
	KUrl base_url = medium.prettyBaseURL();
	QString device_node = medium.deviceNode();

	KFileMetaInfoGroup group = appendGroup(info, "mediumInfo");

	if (base_url.isValid())
	{
		appendItem(group, "baseURL", base_url.prettyUrl());
	}

	if (!device_node.isEmpty())
	{
		appendItem(group, "deviceNode", device_node);
	}

	if (!mount_point.isEmpty() && medium.isMounted())
	{
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

		enterLoop();

		int percent = 0;
		int length = 0;

		if (m_total != 0)
		{
			percent = 100 * m_used / m_total;
			length = 150 * m_used / m_total;
		}

		appendItem(group, "free", (long long unsigned)m_free);
		appendItem(group, "used", (long long unsigned)m_used);
		appendItem(group, "total", (long long unsigned)m_total);

		group = appendGroup(info, "mediumSummary");

		appendItem(group, "percent", QString("%1%").arg(percent));

		QPixmap bar(150, 20);
		QPainter p(&bar);

		p.fillRect(0, 0, length, 20, Qt::red);
		p.fillRect(length, 0, 150-length, 20, Qt::green);

		QColorGroup cg = QApplication::palette().active();
#ifdef __GNUC__
#warning "Port to new QStyle API"
#endif
#if 0
		QApplication::style()->drawPrimitive(QStyle::PE_Frame, &p,
		                                     QRect(0, 0, 150, 20), cg,
		                                     QStyle::State_Sunken);
#endif
		appendItem( group, "thumbnail", bar );
	}

	return true;
}

void KFileMediaPlugin::slotFoundMountPoint(const QString &/*mountPoint*/,
                                            unsigned long total, unsigned long used,
                                            unsigned long free)
{
	m_free = free;
	m_used = used;
	m_total = total;
}

void KFileMediaPlugin::slotDfDone()
{
	emit leaveModality();
}


void KFileMediaPlugin::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

const Medium KFileMediaPlugin::askMedium(KFileMetaInfo &info)
{
  QDBusInterface mediamanager("org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager");
	QDBusReply<QStringList> reply = mediamanager.call( "properties", info.url().fileName() );

	if ( !reply.isValid() )
	{
		return Medium(QString(), QString());
	}

	return Medium::create(reply);
}

void KFileMediaPlugin::addMimeType(const char *mimeType)
{
	KFileMimeTypeInfo *info = addMimeTypeInfo( mimeType );

	KFileMimeTypeInfo::GroupInfo *group
		= addGroupInfo(info, "mediumInfo", i18n("Medium Information"));

	KFileMimeTypeInfo::ItemInfo *item
		= addItemInfo(group, "free", i18n("Free"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KibiBytes);

	item = addItemInfo(group, "used", i18n("Used"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KibiBytes);

	item = addItemInfo(group, "total", i18n("Total"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KibiBytes);

	item = addItemInfo(group, "baseURL", i18n("Base URL"), QVariant::String);
	item = addItemInfo(group, "mountPoint", i18n("Mount Point"), QVariant::String);
	item = addItemInfo(group, "deviceNode", i18n("Device Node"), QVariant::String);

	group = addGroupInfo(info, "mediumSummary", i18n("Medium Summary"));

	item = addItemInfo(group, "percent", i18n("Usage"), QVariant::String);

	item = addItemInfo( group, "thumbnail", i18n("Bar Graph"), QVariant::Image );
	setHint( item, KFileMimeTypeInfo::Thumbnail );
}

#include "kfilemediaplugin.moc"
