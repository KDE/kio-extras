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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kfilemediaplugin.h"

#include <kgenericfactory.h>
#include <kdiskfreesp.h>

#include <dcopref.h>

#include <qpixmap.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qapplication.h>
#include <qeventloop.h>

#include "medium.h"

typedef KGenericFactory<KFileMediaPlugin> KFileMediaPluginFactory;
K_EXPORT_COMPONENT_FACTORY(kfile_media, KFileMediaPluginFactory("kfile_media"))

KFileMediaPlugin::KFileMediaPlugin(QObject *parent, const char *name,
		                     const QStringList& args)
	: KFilePlugin(parent, name, args)
{
	addMimeType( "media/cdrom_mounted" );
	addMimeType( "media/cdwriter_mounted" );
	addMimeType( "media/dvd_mounted" );
	addMimeType( "media/floppy5_mounted" );
	addMimeType( "media/floppy_mounted" );
	addMimeType( "media/hdd_mounted" );
	addMimeType( "media/removable_mounted" );
	addMimeType( "media/zip_mounted" );
}

bool KFileMediaPlugin::readInfo(KFileMetaInfo &info, uint /*what*/)
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

	KFileMetaInfoGroup group = appendGroup(info, "mediumInfo");

	appendItem(group, "free", (long long unsigned)m_free);
	appendItem(group, "used", (long long unsigned)m_used);
	appendItem(group, "total", (long long unsigned)m_total);

	group = appendGroup(info, "mediumSummary");

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
	qApp->eventLoop()->exitLoop();
}

QString KFileMediaPlugin::askMountPoint(KFileMetaInfo &info)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", info.url().fileName() );

	if ( !reply.isValid() )
	{
		return QString::null;
	}

	return Medium::create(reply).mountPoint();
}

void KFileMediaPlugin::addMimeType(const char *mimeType)
{
	KFileMimeTypeInfo *info = addMimeTypeInfo( mimeType );

	KFileMimeTypeInfo::GroupInfo *group
		= addGroupInfo(info, "mediumInfo", i18n("Medium Information"));

	KFileMimeTypeInfo::ItemInfo *item
		= addItemInfo(group, "free", i18n("Free"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);

	item = addItemInfo(group, "used", i18n("Used"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);

	item = addItemInfo(group, "total", i18n("Total"), QVariant::Int);
	setUnit(item, KFileMimeTypeInfo::KiloBytes);


	group = addGroupInfo(info, "mediumSummary", i18n("Medium Summary"));

	item = addItemInfo(group, "percent", i18n("Usage"), QVariant::String);

	item = addItemInfo( group, "thumbnail", i18n("Bar graph"), QVariant::Image );
	setHint( item, KFileMimeTypeInfo::Thumbnail );
}

#include "kfilemediaplugin.moc"
