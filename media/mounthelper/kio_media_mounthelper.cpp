/* This file is part of the KDE project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>
   Parts of this file are
   Copyright 2003 Waldo Bastian <bastian@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <qtimer.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kprocess.h>

#include "kio_media_mounthelper.h"

const Medium MountHelper::findMedium(const QString &name)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( !reply.isValid() )
	{
		m_errorStr = i18n("The KDE mediamanager is not running.")+"\n";
	}

	return Medium::create(reply);
}

MountHelper::MountHelper() : KApplication()
{
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	m_errorStr = "";

	KURL url(args->url(0));
	const Medium medium = findMedium(url.fileName());

	if ( medium.id().isEmpty() )
	{
		m_errorStr+= i18n("%1 cannot be found.").arg(url.prettyURL());
		QTimer::singleShot(0, this, SLOT(error()) );
		return;
	}

	if ( !medium.isMountable() && !args->isSet("e") && !args->isSet("s"))
	{
		m_errorStr = i18n("%1 is not a mountable media.").arg(url.prettyURL());
		QTimer::singleShot(0, this, SLOT(error()) );
		return;
	}

	QString device = medium.deviceNode();
	QString mount_point = medium.mountPoint();

	m_isCdrom = medium.mimeType().find("dvd")!=-1
	         || medium.mimeType().find("cd")!=-1;

	if (args->isSet("u"))
	{
		KIO::Job * job = KIO::unmount( mount_point );

		connect( job, SIGNAL( result( KIO::Job * ) ),
		         this, SLOT( slotResult( KIO::Job * ) ) );
	}
	else if (args->isSet("s"))
	{
		if (medium.isMounted())
		{
			KIO::Job * job = KIO::unmount( mount_point );

			m_device = device;
			connect( job, SIGNAL( result( KIO::Job * ) ),
			         this, SLOT( slotResultSafe( KIO::Job * ) ) );
		}
		else
		{
			invokeEject(device, true);
		}
	}
	else if (args->isSet("e"))
	{
		invokeEject(device);
	}
	else
	{
		 KIO::Job* job = KIO::mount( false, 0, device, mount_point);
		 connect( job, SIGNAL( result( KIO::Job * ) ),
		          this, SLOT( slotResult( KIO::Job * ) ) );
	}
}

void MountHelper::invokeEject(const QString &device, bool quiet)
{
	KProcess *proc = new KProcess(this);
	*proc << "kdeeject";
	if (quiet)
	{
		*proc << "-q";
	}
	*proc << device;
	proc->start();
	connect( proc, SIGNAL(processExited(KProcess *)),
	         this, SLOT( finished() ) );
}

void MountHelper::slotResultSafe(KIO::Job* job)
{
	if (job->error())
	{
		m_errorStr = job->errorText();
		
		if (m_isCdrom)
		{
			m_errorStr+= i18n("\nPlease check that the disk is"
			                  " entered correctly.");
		}
		else
		{
			m_errorStr+= i18n("\nPlease check that the device is"
			                  " plugged correctly.");
		}
		
		QTimer::singleShot(0, this, SLOT(error()) );
	}
	else
	{
		invokeEject(m_device, true);
	}
}

void MountHelper::slotResult(KIO::Job* job)
{
	if (job->error())
	{
		m_errorStr = job->errorText();
		
		if (m_isCdrom)
		{
			m_errorStr+= i18n("\nPlease check that the disk is"
			                  " entered correctly.");
		}
		else
		{
			m_errorStr+= i18n("\nPlease check that the device is"
			                  " plugged correctly.");
		}
				
		QTimer::singleShot(0, this, SLOT(error()) );
	}
	else
	{
		QTimer::singleShot(0, this, SLOT(finished()) );
	}
}

void MountHelper::error()
{
	KMessageBox::error(0, m_errorStr);
	kapp->exit(1);
}

void MountHelper::finished()
{
	kapp->quit();
}

static KCmdLineOptions options[] =
{
	{ "u", I18N_NOOP("Unmount given URL"), 0 },
	{ "m", I18N_NOOP("Mount given URL (default)"), 0 },
	{ "e", I18N_NOOP("Eject given URL via kdeeject"), 0},
	{ "s", I18N_NOOP("Unmount and Eject given URL (necessary for some USB devices)"), 0},
	{"!+URL",   I18N_NOOP("media:/ URL to mount/unmount/eject/remove"), 0 },
	KCmdLineLastOption
};


int main(int argc, char **argv)
{
	KCmdLineArgs::init(argc, argv, "kio_media_mounthelper",
	                   "kio_media_mounthelper", "kio_media_mounthelper",
	                   "0.1");

	KCmdLineArgs::addCmdLineOptions( options );
	KGlobal::locale()->setMainCatalogue("kio_media");
	KApplication::addCmdLineOptions();

	if (KCmdLineArgs::parsedArgs()->count()==0) KCmdLineArgs::usage();
	KApplication *app = new  MountHelper();

	app->dcopClient()->attach();
	app->exec();
}

#include "kio_media_mounthelper.moc"
