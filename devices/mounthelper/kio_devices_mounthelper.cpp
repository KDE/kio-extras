/*
 * Parts of this file are
 * Copyright 2003 Waldo Bastian <bastian@kde.org>
 *
 * These parts are free software; you can redistribute and/or modify
 * them under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 */

#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>
#include <kautomount.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <dcopclient.h>
#include <qtimer.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kprocess.h>

#include "kio_devices_mounthelper.h"

#include "kio_devices_mounthelper.moc"

QStringList KIODevicesMountHelperApp::deviceInfo(QString name)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QStringList retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<name;
	dcopClient()->attach();
        if ( dcopClient()->call( "kded",
                 "mountwatcher", "basicDeviceInfo(QString)", param,retType,data,false ) )
        {
          QDataStream streamin(data,IO_ReadOnly);
          streamin>>retVal;
        }
      return retVal;
}

KIODevicesMountHelperApp::KIODevicesMountHelperApp():KApplication() {
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();;

	KURL url(args->url(0));
	QStringList info=deviceInfo(url.fileName());
//		KMessageBox::information(0,url.url());
	if (info.count() < 3)
	{
//		KMessageBox::information(0,"Wrong data");
		QTimer::singleShot(0,this,SLOT(finished()));
		return;
	}
        QString device=info[1];
	KURL mp=info[2];
	if (args->isSet("u"))
	{
		//KAutoUnmount *um=new KAutoUnmount(mp,QString::null);
		KIO::Job * job = KIO::unmount( mp.path() );
	 	connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
	}
	else if (args->isSet("e"))
	{
		KProcess *proc = new KProcess();
		*proc << "kdeeject";
		*proc << mp.path();
		proc->start();
		connect( proc, SIGNAL(processExited(KProcess *)), this, SLOT( finished() ) );
	}
	else
	{
		 KIO::Job* job = KIO::mount( false, 0, device, mp.path());
		 connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
	}
}

void KIODevicesMountHelperApp::slotResult(KIO::Job* job)
{
	if (job->error())
	{
		errorStr=job->errorText();
		QTimer::singleShot(0,this,SLOT(error()));
	}
	else
	{
//		KDirNotify_stub allDirNotify("*", "KDirNotify*");
//		allDirNotify.FilesAdded( mountpoint );
		QTimer::singleShot(0,this,SLOT(finished()));
	}
}

void KIODevicesMountHelperApp::error()
{
	KMessageBox::error(0,errorStr+i18n("\nPlease check that the disk is entered correctly."));
	kapp->exit(1);
}

void KIODevicesMountHelperApp::finished()
{
	kapp->quit();
}



static KCmdLineOptions options[] =
{
    { "u", I18N_NOOP("Unmount given URL"), 0 },
    { "m", I18N_NOOP("Mount given URL (default)"), 0 },
    { "e", I18N_NOOP("Eject given URL via kdeeject"), 0},
    {"!+[URL]",   I18N_NOOP("devices:/ URL to mount/unmount/eject."), 0 },
    KCmdLineLastOption
};


int main(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "kio_devices_mounthelper", "kio_devices_mounthelper", "0.2");

    KCmdLineArgs::addCmdLineOptions( options );
    KApplication::addCmdLineOptions();

    KApplication *app=new  KIODevicesMountHelperApp();
    KGlobal::locale()->insertCatalogue("kio_devices");

    app->exec();
}
